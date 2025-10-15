#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>   
#include <linux/fcntl.h>    

#define DEVICE_NAME "always_nb"
#define BUF_SIZE 128

static int major;
static char buffer[BUF_SIZE];
static int data_available = 0;

static struct class *nb_class;
static struct device *nb_device;


static ssize_t my_read(struct file *file, char __user *ubuf, size_t len, loff_t *off)
{
    if (data_available == 0) {
        pr_info("No data available — returning -EAGAIN (non-blocking)\n");
        return -EAGAIN;
    }

    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (copy_to_user(ubuf, buffer, len))
        return -EFAULT;

    data_available = 0;
    return len;
}


static ssize_t my_write(struct file *file, const char __user *ubuf, size_t len, loff_t *off)
{
    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (copy_from_user(buffer, ubuf, len))
        return -EFAULT;

    data_available = len;
    pr_info("Data written: %s\n", buffer);

    return len;
}


static int my_open(struct inode *inode, struct file *file)
{
    pr_info("Device opened, forcing non-blocking mode\n");
    file->f_flags |= O_NONBLOCK;  // Force non-blocking
    return 0;
}


static int my_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed, clearing non-blocking flag\n");
    file->f_flags &= ~O_NONBLOCK; // Clear non-blocking on release (optional)
    return 0;
}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = my_read,
    .write = my_write,
    .open = my_open,
    .release = my_release,
};


static int __init nb_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    pr_info("%s: loaded with major %d\n", DEVICE_NAME, major);

    nb_class = class_create(DEVICE_NAME);
    if (IS_ERR(nb_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(nb_class);
    }


    nb_device = device_create(nb_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(nb_device)) {
        class_destroy(nb_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(nb_device);
    }

    pr_info("Device node /dev/%s created\n", DEVICE_NAME);
    return 0;
}


static void __exit nb_exit(void)
{
    device_destroy(nb_class, MKDEV(major, 0));
    class_destroy(nb_class);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("%s: unloaded and device node removed\n", DEVICE_NAME);
}

module_init(nb_init);
module_exit(nb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj");
MODULE_DESCRIPTION("Character driver with automatic /dev node creation and always non-blocking mode");

