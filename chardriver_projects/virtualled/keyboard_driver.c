#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#define DEVICE_NAME "mykbd"
#define BUF_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Raj");
MODULE_DESCRIPTION("Simple Keyboard Char Device Driver");

static dev_t dev;
static struct cdev my_cdev;
static struct class *my_class;

static char buffer[BUF_SIZE];
static int head = 0, tail = 0;
static DEFINE_MUTEX(buf_mutex);

// --- Wait queue for blocking reads ---
static DECLARE_WAIT_QUEUE_HEAD(key_wait);
static int data_available = 0;

// --- US keyboard scancode to lowercase ASCII ---
static const char scancode_to_char[256] = {
    [0x02]='1', [0x03]='2', [0x04]='3', [0x05]='4', [0x06]='5',
    [0x07]='6', [0x08]='7', [0x09]='8', [0x0A]='9', [0x0B]='0',
    [0x10]='q', [0x11]='w', [0x12]='e', [0x13]='r', [0x14]='t',
    [0x15]='y', [0x16]='u', [0x17]='i', [0x18]='o', [0x19]='p',
    [0x1E]='a', [0x1F]='s', [0x20]='d', [0x21]='f', [0x22]='g',
    [0x23]='h', [0x24]='j', [0x25]='k', [0x26]='l',
    [0x2C]='z', [0x2D]='x', [0x2E]='c', [0x2F]='v', [0x30]='b',
    [0x31]='n', [0x32]='m', [0x39]=' ', // space
};

// --- Circular buffer functions ---
static void buffer_put(char c) {
    mutex_lock(&buf_mutex);
    buffer[head] = c;
    head = (head + 1) % BUF_SIZE;
    if (head == tail) tail = (tail + 1) % BUF_SIZE; // overflow
    data_available = 1;
    mutex_unlock(&buf_mutex);
    wake_up_interruptible(&key_wait); // notify waiting readers
}

static int buffer_get(char *c) {
    int ret = 0;
    mutex_lock(&buf_mutex);
    if (head != tail) {
        *c = buffer[tail];
        tail = (tail + 1) % BUF_SIZE;
        if (head == tail) data_available = 0;
        ret = 1;
    }
    mutex_unlock(&buf_mutex);
    return ret;
}

// --- Input event handler ---
static void my_input_event(struct input_handle *handle, unsigned int type,
                           unsigned int code, int value) {
    char c;
    if (type == EV_KEY && value == 1) { // key press only
        c = scancode_to_char[code];
        if (c) buffer_put(c);
    }
}

// --- File operations ---
static int my_open(struct inode *inode, struct file *file) { return 0; }
static int my_release(struct inode *inode, struct file *file) { return 0; }

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char c;
    int count = 0;

    // --- Wait until data is available ---
    wait_event_interruptible(key_wait, data_available);

    while (len-- && buffer_get(&c)) {
        if (copy_to_user(buf++, &c, 1))
            return -EFAULT;
        count++;
    }

    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .release = my_release,
};

// --- Module init and exit ---
static int __init mykbd_init(void) {
    int ret;
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) return ret;

    cdev_init(&my_cdev, &fops);
    cdev_add(&my_cdev, dev, 1);

    my_class = class_create("myclass");
    device_create(my_class, NULL, dev, NULL, DEVICE_NAME);

    printk(KERN_INFO "Simple keyboard driver loaded: /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit mykbd_exit(void) {
    device_destroy(my_class, dev);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "Keyboard driver unloaded\n");
}

module_init(mykbd_init);
module_exit(mykbd_exit);

