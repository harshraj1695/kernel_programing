#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "vled"



#define VLED_MAGIC 'L'
#define VLED_TOGGLE _IO(VLED_MAGIC, 1)

static dev_t dev;
static struct cdev my_cdev;
static struct class *vled_class;


static int led_state = 0;  // 0 = OFF, 1 = ON


static int vled_open(struct inode *inode, struct file *file) {
    pr_info("vled: device opened\n");
    return 0;
}

static int vled_release(struct inode *inode, struct file *file) {
    pr_info("vled: device closed\n");
    return 0;
}

static ssize_t vled_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    if (len < sizeof(int))
        return -EINVAL;

    if (copy_to_user(buf, &led_state, sizeof(int)))
        return -EFAULT;

    pr_info("vled: read state=%d\n", led_state);
    return sizeof(int);
}

static ssize_t vled_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    int val;

    if (len < sizeof(int))
        return -EINVAL;

    if (copy_from_user(&val, buf, sizeof(int)))
        return -EFAULT;

    led_state = val ? 1 : 0;
    pr_info("vled: write new state=%d\n", led_state);

    return sizeof(int);
}

static long vled_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case VLED_TOGGLE:
            led_state = !led_state;
            pr_info("vled: ioctl toggle, new state=%d\n", led_state);
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = vled_open,
    .release        = vled_release,
    .read           = vled_read,
    .write          = vled_write,
    .unlocked_ioctl = vled_ioctl,
};


static int __init vled_init(void) {

    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
        pr_err("vled: failed to allocate device number\n");
        return -1;
    }


    cdev_init(&my_cdev, &fops);
    if (cdev_add(&my_cdev, dev, 1) < 0) {
        pr_err("vled: failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return -1;
    }


    vled_class = class_create(DEVICE_NAME);
    if (IS_ERR(vled_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(vled_class);
    }

    device_create(vled_class, NULL, dev, NULL, DEVICE_NAME);
    pr_info("vled: module loaded, device /dev/%s created\n", DEVICE_NAME);
    return 0;
}


static void __exit vled_exit(void) {
    device_destroy(vled_class, dev);
    class_destroy(vled_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("vled: module unloaded\n");
}

module_init(vled_init);
module_exit(vled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj Singh");
MODULE_DESCRIPTION("Simple Virtual LED Character Driver without private_data");

