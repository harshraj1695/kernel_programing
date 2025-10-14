#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h> // for copy_to_user, copy_from_user

#define DEVICE_NAME "mychardev"
#define CLASS_NAME  "mycharclass"


#define MY_MAGIC 'M'
#define CMD_WRITE_VALUE _IOW(MY_MAGIC, 1, int)
#define CMD_READ_VALUE  _IOR(MY_MAGIC, 2, int)


static int my_value = 0;  // stored value in kernel
static dev_t dev;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

// ioctl handler
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int temp;

    switch (cmd) {
        case CMD_WRITE_VALUE:
            if (copy_from_user(&temp, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            my_value = temp;
            pr_info("IOCTL: Value written = %d\n", my_value);
            break;

        case CMD_READ_VALUE:
            if (copy_to_user((int __user *)arg, &my_value, sizeof(int)))
                return -EFAULT;
            pr_info("IOCTL: Value read = %d\n", my_value);
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

// Basic open/release
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("Device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed\n");
    return 0;
}

// File operations
static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .unlocked_ioctl = my_ioctl,
};

// Module init
static int __init my_init(void)
{
    int ret;

    // 1️⃣ Allocate device numbers
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate major/minor numbers\n");
        return ret;
    }

    // 2️⃣ Initialize and add cdev
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    // 3️⃣ Create class
    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class)) {
        pr_err("Failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(my_class);
    }

    // 4️⃣ Create device node automatically
    my_device = device_create(my_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("Failed to create device\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(my_device);
    }

    pr_info("IOCTL driver loaded successfully!\n");
    pr_info("Device created: /dev/%s (Major=%d, Minor=%d)\n",
            DEVICE_NAME, MAJOR(dev), MINOR(dev));

    return 0;
}

// Module exit
static void __exit my_exit(void)
{
    device_destroy(my_class, dev);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("IOCTL driver unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj");
MODULE_DESCRIPTION("Simple ioctl character driver with class/device creation");

