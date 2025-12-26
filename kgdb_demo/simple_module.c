#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init simple_init(void)
{
    printk(KERN_INFO "Hello, KGDB Demo Module Loaded!\n");

    // Simple test variable for debugging
    int x = 5;
    int y = 10;
    int sum = x + y;

    printk(KERN_INFO "Sum of %d + %d = %d\n", x, y, sum);

    return 0;
}

static void __exit simple_exit(void)
{
    printk(KERN_INFO "Goodbye, KGDB Demo Module Unloaded!\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj");
MODULE_DESCRIPTION("Simple KGDB Debug Demo Module");
