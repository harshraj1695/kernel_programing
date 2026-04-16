#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>

#define VENDOR_ID 0x10ec
#define DEVICE_ID 0x8139
#define RX_BUF_SIZE 8192

// --- THE BACKPACK (Private Data) ---
struct my_rtl8139_private {
    void __iomem *ioaddr;
    void *rx_buffer;
    dma_addr_t rx_dma_handle;
};

// --- THE INTERRUPT HANDLER (The "Ear") ---
static irqreturn_t my_rtl8139_interrupt(int irq, void *dev_instance) {
    struct my_rtl8139_private *priv = dev_instance;
    u16 status;
    u16 packet_len;
    unsigned char *raw_packet;
    int i;

    // 1. Read the Interrupt Status Register (0x3E)
    status = ioread16(priv->ioaddr + 0x3E);
    
    // If no interrupt bits are set, it's not our business
    if (status == 0) return IRQ_NONE;

    // 2. Handle "Receive OK" (Bit 0)
    if (status & 0x01) {
        // The RTL8139 puts a 4-byte header at the start of the packet:
        // Bytes 0-1: Status bits
        // Bytes 2-3: Packet Length (Little Endian)
        packet_len = *(u16 *)(priv->rx_buffer + 2);
        
        // The real Ethernet data starts AFTER the 4-byte header
        raw_packet = (unsigned char *)priv->rx_buffer + 4;

        printk(KERN_INFO "RTL8139: Captured %d bytes. Byte Stream:\n", packet_len);

        // 3. Print the Byte Stream (Raw Hex)
        // We limit to 32 bytes to keep dmesg clean, but you can increase this.
        for (i = 0; i < (packet_len > 32 ? 32 : packet_len); i++) {
            pr_cont("%02x ", raw_packet[i]);
        }
        pr_cont("\n");

        // 4. Update CAPR (0x38) - The "Read Pointer"
        // Writing 0 resets the ring buffer pointer for our testing phase.
        iowrite16(0, priv->ioaddr + 0x38);
    }

    // 5. Acknowledge: Clear the bits we handled by writing them back to ISR
    iowrite16(status, priv->ioaddr + 0x3E);

    return IRQ_HANDLED;
}

static int my_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
    struct my_rtl8139_private *priv;
    void __iomem *ioaddr;
    int i;
    u8 mac[6];

    printk(KERN_INFO "RTL8139: Starting custom probe...\n");

    // 1. Setup Private Data structure
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) return -ENOMEM;
    pci_set_drvdata(pdev, priv);

    // 2. Enable Hardware & DMA
    if (pci_enable_device(pdev)) return -ENODEV;
    pci_set_master(pdev); 

    // 3. Map Registers (BAR 1 is usually the MMIO region)
    ioaddr = pci_iomap(pdev, 1, 0);
    if (!ioaddr) return -ENOMEM;
    priv->ioaddr = ioaddr;

    // 4. Read Hardware MAC Address (Offset 0x00 - 0x05)
    for (i = 0; i < 6; i++) mac[i] = ioread8(ioaddr + i);
    printk(KERN_INFO "RTL8139: Hardware MAC: %pM\n", mac);

    // 5. Software Reset
    iowrite8(0x10, ioaddr + 0x37);
    while (ioread8(ioaddr + 0x37) & 0x10) { cpu_relax(); }

    // 6. Allocate DMA Buffer (8K + 16 bytes for safety)
    priv->rx_buffer = dma_alloc_coherent(&pdev->dev, RX_BUF_SIZE + 16, 
                                        &priv->rx_dma_handle, GFP_KERNEL);
    if (!priv->rx_buffer) return -ENOMEM;

    // 7. Tell the chip where to write packets (RBSTART - 0x30)
    iowrite32((u32)priv->rx_dma_handle, ioaddr + 0x30);

    // 8. Register Interrupt Handler
    if (request_irq(pdev->irq, my_rtl8139_interrupt, IRQF_SHARED, "my_rtl8139", priv)) {
        return -EIO;
    }

    // 9. Configure Receiver (RCR - 0x44)
    // 0x0000c00f = Accept All (Promiscuous) + Broadcast + Multicast + 8K Buffer
    iowrite32(0x0000c00f, ioaddr + 0x44);

    // 10. Enable Interrupt Mask (IMR - 0x3C)
    // 0x0005 = Enable Receive OK (ROK) and Transmit OK (TOK)
    iowrite16(0x0005, ioaddr + 0x3C);

    // 11. Start Engines (RE - Receive Enable)
    iowrite8(0x0C, ioaddr + 0x37);

    printk(KERN_INFO "RTL8139: Driver Active on IRQ %d\n", pdev->irq);
    return 0;
}

static void my_remove(struct pci_dev *pdev) {
    struct my_rtl8139_private *priv = pci_get_drvdata(pdev);

    if (priv) {
        // Turn off chip engines and interrupts
        iowrite8(0x00, priv->ioaddr + 0x37);
        iowrite16(0x0000, priv->ioaddr + 0x3C);

        free_irq(pdev->irq, priv);
        
        if (priv->rx_buffer) {
            dma_free_coherent(&pdev->dev, RX_BUF_SIZE + 16, 
                              priv->rx_buffer, priv->rx_dma_handle);
        }
        pci_iounmap(pdev, priv->ioaddr);
    }
    pci_disable_device(pdev);
    printk(KERN_INFO "RTL8139: Driver Unloaded.\n");
}

static struct pci_device_id rtl8139_table[] = {
    { PCI_DEVICE(VENDOR_ID, DEVICE_ID), },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, rtl8139_table);

static struct pci_driver my_driver = {
    .name = "my_rtl8139_driver",
    .id_table = rtl8139_table,
    .probe = my_probe,
    .remove = my_remove,
};

module_pci_driver(my_driver);
MODULE_LICENSE("GPL");