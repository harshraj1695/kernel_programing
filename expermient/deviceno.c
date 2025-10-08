#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj Singh");
MODULE_DESCRIPTION("Simple dynamic char device allocation");

static dev_t dev;

static int __init mymodule_init(void) {
    int ret;
    printk(KERN_INFO "Going in init module\n");

    ret = alloc_chrdev_region(&dev, 0, 1, "mychardev");
    if (ret < 0) {
        printk(KERN_ERR "Error occurred\n");
        return ret;
    }

    printk(KERN_INFO "Major: %d, Minor: %d\n", MAJOR(dev), MINOR(dev));
    return 0;
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "Exiting module...\n");
    unregister_chrdev_region(dev, 1);
}

module_init(mymodule_init);
module_exit(mymodule_exit);

