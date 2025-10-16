#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"

#define MAGIC 'M'
#define CMD_WRITE _IOW(MAGIC, 1, int)
#define CMD_READ  _IOR(MAGIC, 2, int)

static int value = 0;
static dev_t dev;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;
static int open_count = 0;


struct mydev1{
        int a;
        int b;
};


static int my_open(struct inode *inode, struct file *file)
{
    open_count++;
    printk(KERN_INFO "mykbd: device opened %d times\n", open_count);
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    if (open_count > 0)
        open_count--;
    printk(KERN_INFO "mykbd: device still opened by %d processes\n", open_count);
    return 0;
}


static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        struct mydev1 news1;
    switch (cmd) {
        case CMD_WRITE:
            if (copy_from_user(&news1, (struct mydev1 __user *)arg, sizeof(struct mydev1)))
                return -EFAULT;
            pr_info("IOCTL: Value written = %d, %d\n", news1.a, news1.b);
            break;

        case CMD_READ:
            value=1;
            if (copy_to_user((int __user *)arg, &value, sizeof(int)))
                return -EFAULT;
            pr_info("IOCTL: Value read = %d\n", value);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .unlocked_ioctl = my_ioctl,
};

static int __init my_init(void)
{
    int ret;


    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate major/minor numbers\n");
        return ret;
    }


    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }


    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class)) {
        pr_err("Failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(my_class);
    }


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

