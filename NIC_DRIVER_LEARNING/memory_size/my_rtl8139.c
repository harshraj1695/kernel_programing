#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/pci.h>

#define VENDOR_ID 0x10ec
#define DEVICE_ID 0x8139

// --- THE BACKPACK ---
// This structure holds everything unique to ONE specific network card
struct my_rtl8139_private {
  void __iomem *ioaddr;
  void *rx_buffer;
  dma_addr_t rx_dma_handle;
};

static struct pci_device_id rtl8139_table[] = {
    {
        PCI_DEVICE(VENDOR_ID, DEVICE_ID),
    },
    {
        0,
    }};
MODULE_DEVICE_TABLE(pci, rtl8139_table);

static int my_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
  struct my_rtl8139_private *priv;
  void __iomem *ioaddr;
  u8 mac[6];
  int i;

  printk(KERN_INFO "RTL8139: Probe started.\n");

  // 1. Allocate the Backpack (Private Data)
  // devm_kzalloc is "managed" memory—it cleans itself up if probe fails!
  priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  // 2. Attach the backpack to the PCI device
  pci_set_drvdata(pdev, priv);

  // 3. Enable Hardware
  if (pci_enable_device(pdev))
    return -ENODEV;

  // 4. Map the Registers
  ioaddr = pci_iomap(pdev, 1, 0);
  if (!ioaddr)
    return -ENOMEM;
  priv->ioaddr = ioaddr; // Save it in the backpack!

  // 5. Read MAC
  for (i = 0; i < 6; i++) {
    mac[i] = ioread8(ioaddr + i);
  }
  printk(KERN_INFO "RTL8139: MAC Address: %pM\n", mac);

  // 6. Reset Hardware
  iowrite8(0x10, ioaddr + 0x37);
  while (ioread8(ioaddr + 0x37) & 0x10) {
    /* Wait for reset completion */
  }
  printk(KERN_INFO "RTL8139: Reset complete.\n");

  // 7. Allocate DMA Buffer
  // We store the pointers directly in the backpack
  priv->rx_buffer = dma_alloc_coherent(&pdev->dev, 8192 + 16,
                                       &priv->rx_dma_handle, GFP_KERNEL);
  if (!priv->rx_buffer) {
    return -ENOMEM;
  }

  printk(KERN_INFO "RTL8139: Rx Buffer Virtual: %p\n", priv->rx_buffer);
  printk(KERN_INFO "RTL8139: Rx Buffer Physical: %pad\n", &priv->rx_dma_handle);

  // 8. Tell the chip where the buffer is (RBSTART)
  iowrite32((u32)priv->rx_dma_handle, ioaddr + 0x30);

  u32 rcr_val;

  // 1. Start with the "Accept" bits
  // We want: Broadcast (0x08) + Multicast (0x04) + Physical Match (0x02)
  rcr_val = 0x08 | 0x04 | 0x02;

  // 2. Set the Buffer Size (8K is 00 at bits 11 and 12, so we add nothing)
  // 3. Set the FIFO threshold (Setting it to 'Unlimited' or 1024)
  rcr_val |= (0x06 << 13); // 0x06 is the '1024' code in the datasheet

  // 4. Write it to the chip (Offset 0x44)
  iowrite32(rcr_val, priv->ioaddr + 0x44);

  printk(KERN_INFO "RTL8139: RCR (0x44) configured with value: 0x%08x\n",
         rcr_val);

  // Enable Receiver (RE - Bit 3) and Transmitter (TE - Bit 2)
  // Binary 0000 1100 = 0x0C
  iowrite8(0x0C, priv->ioaddr + 0x37);

  printk(KERN_INFO "RTL8139: Receiver and Transmitter engines are NOW ON!\n");

  return 0;
}

static void my_remove(struct pci_dev *pdev) {
  // 1. Retrieve the Backpack we saved in probe
  struct my_rtl8139_private *priv = pci_get_drvdata(pdev);

  if (priv) {
    // 2. Use the addresses stored in the backpack to free memory
    if (priv->rx_buffer) {
      dma_free_coherent(&pdev->dev, 8192 + 16, priv->rx_buffer,
                        priv->rx_dma_handle);
      printk(KERN_INFO "RTL8139: Freed DMA buffer.\n");
    }

    // 3. Unmap the IO registers
    if (priv->ioaddr) {
      pci_iounmap(pdev, priv->ioaddr);
    }
  }

  pci_disable_device(pdev);
  printk(KERN_INFO "RTL8139: Driver removed.\n");
}

static struct pci_driver my_driver = {
    .name = "my_rtl8139_driver",
    .id_table = rtl8139_table,
    .probe = my_probe,
    .remove = my_remove,
};

module_pci_driver(my_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTL8139 learning driver with PCI MMIO and DMA setup");
