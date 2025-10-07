#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>        // alloc_chrdev_region, unregister_chrdev_region

static dev_t dev_num;        // holds major+minor
static int count = 1;        // number of devices we want to allocate
static char *device_name = "mychardev";

static int __init mychardev_init(void)
{
    int ret;

    // Dynamically allocate a device number (major + minor)
    ret = alloc_chrdev_region(&dev_num, 0, count, device_name);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    pr_info("Device registered: %s\n", device_name);
    pr_info("Major = %d, Minor = %d\n", MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit mychardev_exit(void)
{
    // Free the device numbers
    unregister_chrdev_region(dev_num, count);
    pr_info("Device unregistered: %s\n", device_name);
}

module_init(mychardev_init);
module_exit(mychardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj Singh");
MODULE_DESCRIPTION("Example module for dynamic device number allocation");

