#include <linux/kernel.h>    // Kernel logging (pr_info, printk)
#include <linux/init.h>      // __init, __exit macros
#include <linux/module.h>    // Core header for modules

// Init function - runs when module is loaded
static int __init my_module_init(void)
{
    pr_info("Hi from Harsh! Kernel module loaded.\n");
    return 0;
}

// Exit function - runs when module is removed
static void __exit my_module_exit(void)
{
    pr_info("Goodbye from kernel, Harsh!\n");
}

// Register init and exit functions
module_init(my_module_init);
module_exit(my_module_exit);

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj Singh");
MODULE_DESCRIPTION("Simple module example");

