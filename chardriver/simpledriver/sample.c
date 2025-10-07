#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj");
MODULE_DESCRIPTION("Minimal working char driver");

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *cls;

static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychar: Device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychar: Device closed\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char msg[] = "Jai Bajrang Bali!\n";
    size_t msg_len = sizeof(msg) - 1;
    size_t remaining = msg_len - *off;
    size_t to_copy = (len < remaining) ? len : remaining;

    if (to_copy == 0)
        return 0;

    if (copy_to_user(buf, msg + *off, to_copy))
        return -EFAULT;

    *off += to_copy;
    return to_copy;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char *kbuf;

    kbuf = kmalloc(len + 1, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kbuf[len] = '\0';  // Null-terminate
    printk(KERN_INFO "mychar: Written: %s\n", kbuf);
    kfree(kbuf);

    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_init(void)
{
    if (alloc_chrdev_region(&dev_num, 0, 1, "mychar") < 0)
        return -1;

    printk(KERN_INFO "mychar: Major = %d Minor = %d\n", MAJOR(dev_num), MINOR(dev_num));

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    if (cdev_add(&my_cdev, dev_num, 1) < 0) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    cls = class_create("mychar_class");
    device_create(cls, NULL, dev_num, NULL, "mychar");

    printk(KERN_INFO "mychar: Device created successfully\n");
    return 0;
}

static void __exit my_exit(void)
{
    device_destroy(cls, dev_num);
    class_destroy(cls);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "mychar: Device removed\n");
}

module_init(my_init);
module_exit(my_exit);

