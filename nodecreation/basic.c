#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HARSH RAJ");
MODULE_DESCRIPTION("Dynamic device node example without cdev");

static dev_t dev;
static struct class *cls;

static int __init my_init(void)
{
    int ret;

    // Allocate a dynamic major number
    ret = alloc_chrdev_region(&dev, 0, 1, "mydynchar");
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    // Create class for automatic /dev node creation
    cls = class_create("mydyn_class");
if (IS_ERR(cls)) {
    unregister_chrdev_region(dev, 1);
    pr_err("Failed to create class\n");
    return PTR_ERR(cls);
}


    // Create /dev node dynamically
    device_create(cls, NULL, dev, NULL, "mydyn0");

    pr_info("Dynamic device node created: /dev/mydyn0\n");
    return 0;
}

static void __exit my_exit(void)
{
    // Remove device node and class
    device_destroy(cls, dev);
    class_destroy(cls);

    // Release major number
    unregister_chrdev_region(dev, 1);

    pr_info("Dynamic device node removed\n");
}

module_init(my_init);
module_exit(my_exit);

