#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/init.h>

static struct work_struct my_work;

void my_work_handler(struct work_struct *work)
{
    printk(KERN_INFO "Work executed by kernel thread\n");
}

static int __init my_init(void)
{
    INIT_WORK(&my_work, my_work_handler);
    schedule_work(&my_work);   // Queues it for execution
    return 0;
}

static void __exit my_module_exit(void)
{

    printk(KERN_INFO "Interrupt handler freed\n");
}

module_init(my_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");

