#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#define IRQ_NUM 1  
static int irq = IRQ_NUM;
static int shared_data = 0;

// Declare tasklet
static void my_tasklet_handler(unsigned long data);
DECLARE_TASKLET(my_tasklet, my_tasklet_handler, 0);


// Top Half Interrupt Handler

static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    pr_info("Top Half: Interrupt %d triggered!\n", irq);

    // Simulate reading from device
    shared_data++;

    // Schedule bottom half (tasklet)
    tasklet_schedule(&my_tasklet);

    return IRQ_HANDLED;
}


// Bottom Half Tasklet Handler
static void my_tasklet_handler(unsigned long data)
{
    pr_info("Bottom Half (Tasklet): running now.\n");
    pr_info("Tasklet processed shared_data = %d\n", shared_data);
}


static int __init my_init(void)
{
    int ret;
    pr_info("Loading Tasklet IRQ Module...\n");

    // Register IRQ handler
    ret = request_irq(irq, my_irq_handler, IRQF_SHARED, "my_irq_tasklet", &irq);
    if (ret) {
        pr_err("Failed to register IRQ %d\n", irq);
        return ret;
    }

    pr_info("Registered IRQ handler on IRQ %d\n", irq);
    return 0;
}

static void __exit my_exit(void)
{
    tasklet_kill(&my_tasklet);  // Ensure tasklet done
    free_irq(irq, &irq);
    pr_info("Tasklet IRQ Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Example");
MODULE_DESCRIPTION("Demo: Top half + Bottom half (Tasklet) Interrupt handling");

