#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/mutex.h>

#define DEVICE_NAME "block_nb"
#define BUF_SIZE 128

static int major;
static char buffer[BUF_SIZE];
static int data_available = 0;

static struct class *blk_class;
static struct device *blk_device;

/* Wait queue and mutex */
static DECLARE_WAIT_QUEUE_HEAD(read_queue);
static DEFINE_MUTEX(buf_mutex);


static ssize_t my_read(struct file *file, char __user *ubuf, size_t len, loff_t *off)
{
    /* Wait until data is available */
    if (wait_event_interruptible(read_queue, data_available > 0))
        return -ERESTARTSYS; // interrupted by signal

    mutex_lock(&buf_mutex);

    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (copy_to_user(ubuf, buffer, len)) {
        mutex_unlock(&buf_mutex);
        return -EFAULT;
    }

    data_available = 0; // data consumed
    mutex_unlock(&buf_mutex);

    return len;
}


static ssize_t my_write(struct file *file, const char __user *ubuf, size_t len, loff_t *off)
{
    if (len > BUF_SIZE)
        len = BUF_SIZE;

    mutex_lock(&buf_mutex);

    if (copy_from_user(buffer, ubuf, len)) {
        mutex_unlock(&buf_mutex);
        return -EFAULT;
    }

    data_available = len;
    pr_info("Data written: %s\n", buffer);

    mutex_unlock(&buf_mutex);

    wake_up_interruptible(&read_queue); // wake any blocked readers
    return len;
}

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


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = my_read,
    .write = my_write,
    .open = my_open,
    .release = my_release,
};


static int __init blk_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    pr_info("%s: loaded with major %d\n", DEVICE_NAME, major);


    blk_class = class_create(DEVICE_NAME);
    if (IS_ERR(blk_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(blk_class);
    }


    blk_device = device_create(blk_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(blk_device)) {
        class_destroy(blk_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(blk_device);
    }

    pr_info("Device node /dev/%s created\n", DEVICE_NAME);
    return 0;
}


static void __exit blk_exit(void)
{
    device_destroy(blk_class, MKDEV(major, 0));
    class_destroy(blk_class);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("%s: unloaded and device node removed\n", DEVICE_NAME);
}

module_init(blk_init);
module_exit(blk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj");
MODULE_DESCRIPTION("Blocking character driver with automatic /dev node creation");

