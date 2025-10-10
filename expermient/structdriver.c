#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE "mychardev"

static dev_t dev;
static struct cdev my_cdev;
static struct class *classptr;
static struct device *deviceptr;

struct student {
    int a;
    int b;
};

struct student *kbuff;

static int my_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "open file function called\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "release function called\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *userbuff, size_t len, loff_t *offset) {
    printk(KERN_INFO "write function called\n");

    if (copy_from_user(kbuff, userbuff, sizeof(struct student))) {
        printk(KERN_ERR "Failed to copy data from user\n");
        return -EFAULT;
    }

    printk(KERN_INFO "Received struct from user: a=%d, b=%d\n", kbuff->a, kbuff->b);
    return sizeof(struct student);
}

static ssize_t my_read(struct file *file, char __user *userbuff, size_t len, loff_t *offset) {
    if (*offset > 0)
        return 0;  // EOF

    if (copy_to_user(userbuff, kbuff, sizeof(struct student))) {
        printk(KERN_ERR "Failed to copy data to user\n");
        return -EFAULT;
    }

    printk(KERN_INFO "Sent struct to user: a=%d, b=%d\n", kbuff->a, kbuff->b);
    *offset += sizeof(struct student);
    return sizeof(struct student);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init mymodule(void) {
    alloc_chrdev_region(&dev, 0, 1, DEVICE);
    printk(KERN_INFO "Major: %d, Minor: %d\n", MAJOR(dev), MINOR(dev));

    cdev_init(&my_cdev, &fops);
    cdev_add(&my_cdev, dev, 1);

    classptr = class_create(DEVICE);
    deviceptr = device_create(classptr, NULL, dev, NULL, DEVICE);

    kbuff = kmalloc(sizeof(struct student), GFP_KERNEL);
    printk(KERN_INFO "Device created successfully\n");
    return 0;
}

static void __exit myexit(void) {
    kfree(kbuff);
    device_destroy(classptr, dev);
    class_destroy(classptr);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "Driver unloaded\n");
}

module_init(mymodule);
module_exit(myexit);

MO

