#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#define IRQ_NO 11

static struct work_struct my_work;
void work_handler(struct work_struct *work);


void work_handler(struct work_struct *work)
{
    printk(KERN_INFO "Work running asynchronously in process context\n");
    msleep(500);  // OK here (can sleep)
    printk(KERN_INFO "Work done, now returning\n");
}

static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "ISR running (atomic context, cannot sleep)\n");


    schedule_work(&my_work);

    printk(KERN_INFO "ISR returning quickly (work will run later)\n");
    return IRQ_HANDLED;
}

static int __init my_init(void)
{
    int ret;

    INIT_WORK(&my_work, work_handler);
    ret = request_irq(IRQ_NO, my_irq_handler, IRQF_SHARED, "async_demo", &my_work);

    if (ret) {
        printk(KERN_ERR "request_irq failed: %d\n", ret);
        return ret;  // Fail module load
    }

    printk(KERN_INFO "Module loaded. Simulating interrupt...\n");

    my_irq_handler(IRQ_NO, NULL);
    return 0;
}


static void __exit my_exit(void)
{
    //flush_scheduled_work();
    free_irq(IRQ_NO, &my_work);
    printk(KERN_INFO "Module unloaded.\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

