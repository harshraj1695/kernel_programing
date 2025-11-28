#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#define RING_SIZE 4
#define BUF_SIZE 64

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh");
MODULE_DESCRIPTION("Simple RX interrupt-driven simulation");

// Simulated descriptor
typedef struct {
    char buffer[BUF_SIZE];
    int length;
    int own;  // 1=HW owns, 0=CPU owns
} rx_desc;

static rx_desc *ring;
static int hw_index = 0;
static int cpu_index = 0;
static int irq_number = 11; // hypothetical IRQ number

// Simulate hardware writing packets
static void simulate_hw_packet(void) {
    if (ring[hw_index].own == 1) {
        snprintf(ring[hw_index].buffer, BUF_SIZE, "Packet %d", hw_index + 1);
        ring[hw_index].length = strlen(ring[hw_index].buffer);
        ring[hw_index].own = 0; // CPU can now read
        printk(KERN_INFO "HW: wrote %s at desc %d\n", ring[hw_index].buffer, hw_index);
        hw_index = (hw_index + 1) % RING_SIZE;
    }
}

// Interrupt handler
static irqreturn_t rx_isr(int irq, void *dev_id) {
    while (ring[cpu_index].own == 0) {
        printk(KERN_INFO "CPU ISR: processing %s from desc %d\n",
               ring[cpu_index].buffer, cpu_index);
        ring[cpu_index].own = 1; // give ownership back to hardware
        cpu_index = (cpu_index + 1) % RING_SIZE;
    }
    return IRQ_HANDLED;
}

static int __init rx_module_init(void) {
    int i, ret;

    // Allocate RX ring
    ring = kzalloc(sizeof(rx_desc) * RING_SIZE, GFP_KERNEL);
    if (!ring)
        return -ENOMEM;

    for (i = 0; i < RING_SIZE; i++)
        ring[i].own = 1; // HW owns initially

    // Register IRQ
    ret = request_irq(irq_number, rx_isr, IRQF_SHARED, "rx_sim_irq", (void *)ring);
    if (ret) {
        kfree(ring);
        printk(KERN_ERR "Failed to request IRQ\n");
        return ret;
    }

    printk(KERN_INFO "RX module loaded\n");

    // Simulate a few packet arrivals
    simulate_hw_packet();
    simulate_hw_packet();
    // In real driver, HW triggers interrupt after writing packets
    rx_isr(irq_number, ring); // manually call ISR to simulate interrupt

    return 0;
}

static void __exit rx_module_exit(void) {
    free_irq(irq_number, (void *)ring);
    kfree(ring);
    printk(KERN_INFO "RX module unloaded\n");
}

module_init(rx_module_init);
module_exit(rx_module_exit);

