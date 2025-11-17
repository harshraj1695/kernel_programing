#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "mynode"
#define NODE_COUNT  2
#define BUF_SIZE    100

static dev_t dev_num;
static struct class *cls;
static struct cdev my_cdev;

struct my_data {
    char buf[BUF_SIZE];
    int len;
    
};

static int my_open(struct inode *inode, struct file *file)
{
    struct my_data *pdata = kmalloc(sizeof(struct my_data), GFP_KERNEL);
    pdata->len = 0;
    //memset(pdata->buf, 0, BUF_SIZE);
    file->private_data = pdata;
    printk(KERN_INFO "mynode%d opened\n", MINOR(inode->i_rdev));
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    printk(KERN_INFO "mynode%d closed\n", MINOR(inode->i_rdev));
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
    struct my_data *pdata = file->private_data;

    if (count > BUF_SIZE)
        count = BUF_SIZE;

    if (copy_from_user(pdata->buf, buf, count))
        return -EFAULT;

    pdata->len = count;

    *ppos = 0;   // IMPORTANT: reset read pointer for next read()

    return count;
}


static ssize_t my_read(struct file *file, char __user *user_buffer,
                       size_t count, loff_t *offset)
{
    struct my_data *pdata = file->private_data;
    int bytes_to_read;
    int ret;


    if (*offset >= pdata->len)
        return 0;   // EOF


    bytes_to_read = pdata->len - *offset;

    if (count < bytes_to_read)
        bytes_to_read = count;


    ret = copy_to_user(user_buffer, pdata->buf + *offset, bytes_to_read);
    if (ret != 0)
        return -EFAULT;

    *offset += bytes_to_read;

    return bytes_to_read;
}



static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static int __init my_init(void)
{
    int i;
    alloc_chrdev_region(&dev_num, 0, NODE_COUNT, DEVICE_NAME);
    cdev_init(&my_cdev, &fops);
    cdev_add(&my_cdev, dev_num, NODE_COUNT);

    cls = class_create( DEVICE_NAME);
    for (i = 0; i < NODE_COUNT; i++)
        device_create(cls, NULL, dev_num + i, NULL, "mynode%d", i);
    printk(KERN_INFO "mynode driver loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    int i;
    for (i = 0; i < NODE_COUNT; i++)
        device_destroy(cls, dev_num + i);

    class_destroy(cls);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, NODE_COUNT);
    printk(KERN_INFO "mynode driver unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

