#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#define MY_IRQ 1   // Example: IRQ 1 = keyboard on x86 (just for demo)

static irqreturn_t my_interrupt_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "Interrupt occurred! IRQ=%d\n", irq);
    return IRQ_HANDLED; // tell kernel we handled it
}

static int __init my_module_init(void)
{
    int result;

    result = request_irq(MY_IRQ, my_interrupt_handler,
                         IRQF_SHARED, "my_irq_handler", (void *)(my_interrupt_handler));

    if (result) {
        printk(KERN_ERR "Failed to register interrupt %d\n", MY_IRQ);
        return result;
    }

    printk(KERN_INFO "Interrupt handler registered for IRQ %d\n", MY_IRQ);
    return 0;
}

static void __exit my_module_exit(void)
{
    free_irq(MY_IRQ, (void *)(my_interrupt_handler));
    printk(KERN_INFO "Interrupt handler freed\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");

