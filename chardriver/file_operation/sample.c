#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

static dev_t dev_num;           
static struct cdev my_cdev;      
static struct class *cls;      

static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char msg[] = "Jai Bajrang Bali!\n";
    int bytes = strlen(msg);

    if (*off >= bytes) return 0;
    if (copy_to_user(buf, msg, bytes)) return -EFAULT;

    *off += bytes;
    return bytes;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .release = my_release,
};

static int __init my_init(void)
{
    //Allocate major+minor dynamically
    if (alloc_chrdev_region(&dev_num, 0, 1, "mychar") < 0)
        return -1;

    printk(KERN_INFO "Major = %d, Minor = %d\n", MAJOR(dev_num), MINOR(dev_num));

    // Initialize cdev structure with file operations
    cdev_init(&my_cdev, &fops);

    // Add cdev to the system
    if (cdev_add(&my_cdev, dev_num, 1) < 0) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    //Create class and device node automatically in /dev/
cls = class_create("myclass");

    device_create(cls, NULL, dev_num, NULL, "mychar");

    printk(KERN_INFO "Device created successfully\n");
    return 0;
}

static void __exit my_exit(void)
{
    device_destroy(cls, dev_num);
    class_destroy(cls);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "Device removed\n");
}

module_init(my_init);
module_exit(my_exit);

