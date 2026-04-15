#include <linux/module.h>
#include <linux/pci.h>

// Realtek 8139 IDs
#define VENDOR_ID 0x10ec
#define DEVICE_ID 0x8139

static struct pci_device_id rtl8139_table[] = {
    {
        PCI_DEVICE(VENDOR_ID, DEVICE_ID),
    },
    {
        0,
    }};
MODULE_DEVICE_TABLE(pci, rtl8139_table);

static int my_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
  printk(KERN_INFO "RTL8139: Found the NIC! Mapping registers...\n");

  // Step 1: Enable the device
  if (pci_enable_device(pdev))
    return -ENODEV;

  // Step 2: Get the Base Address of the registers (BAR 1 is usually MMIO)
  unsigned long mmio_start = pci_resource_start(pdev, 1);
  unsigned long mmio_len = pci_resource_len(pdev, 1);

  // Step 3: Map that physical hardware address into virtual kernel memory
  void __iomem *ioaddr = pci_iomap(pdev, 1, 0);

  if (!ioaddr)
    return -ENOMEM;

  // Step 4: Read the MAC Address (Registers 0-5)
  u8 mac[6];
  int i;
  for (i = 0; i < 6; i++) {
    mac[i] = ioread8(ioaddr + i);
  }

  printk(KERN_INFO "RTL8139: MAC Address is %pM\n", mac);

  u8 reg_val;

  // 1. Print the "First State" (Before Reset)
  reg_val = ioread8(ioaddr + 0x37);
  printk(KERN_INFO "RTL8139: Command Reg before reset: 0x%02x\n", reg_val);

  // 2. Toggle the Reset Bit (Bit 4 = 0x10)
  printk(KERN_INFO "RTL8139: Pushing the Reset button now...\n");
  iowrite8(0x10, ioaddr + 0x37);


  // 3. Check the state immediately after
  reg_val = ioread8(ioaddr + 0x37);
  printk(KERN_INFO "RTL8139: Command Reg immediately after: 0x%02x\n", reg_val);

  // 4. The "Move" (Polling)
  // We wait for the hardware to clear the bit (flip it back to 0)
  while (ioread8(ioaddr + 0x37) & 0x10) {
    // In a nursery driver, we just loop.
    // In a professional driver, we'd add a small delay or a timeout.
  }

  printk(KERN_INFO "RTL8139: Hardware has finished resetting! Bit is now 0.\n");

  return 0;
}

static void my_remove(struct pci_dev *pdev) {
  printk(KERN_INFO "RTL8139: Driver unloaded.\n");
}

static struct pci_driver my_driver = {
    .name = "my_rtl8139_driver",
    .id_table = rtl8139_table,
    .probe = my_probe,
    .remove = my_remove,
};

module_pci_driver(my_driver);
MODULE_LICENSE("GPL");
