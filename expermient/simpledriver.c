#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh");
MODULE_DESCRIPTION("Simple Character Driver Example");
MODULE_VERSION("1.0");

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;
static char *kernel_buffer;

// ---------------- File Operations ----------------
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mychardev: Device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mychardev: Device closed\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int bytes_to_copy, bytes_not_copied;

    // If data already read, return 0 (EOF)
    if (*offset > 0)
        return 0;

    bytes_to_copy = min(len, (size_t)strlen(kernel_buffer));

    bytes_not_copied = copy_to_user(buf, kernel_buffer, bytes_to_copy);

    // Update offset so next read returns 0
    *offset += bytes_to_copy - bytes_not_copied;

    pr_info("mychardev: Sent %d bytes to user\n", bytes_to_copy - bytes_not_copied);

    return bytes_to_copy - bytes_not_copied;
}


static ssize_t my_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    int bytes_not_copied;
    memset(kernel_buffer, 0, BUFFER_SIZE);
    len = min(len, (size_t)(BUFFER_SIZE - 1));
    bytes_not_copied = copy_from_user(kernel_buffer, buf, len);
    pr_info("mychardev: Received %zu bytes from user\n", len - bytes_not_copied);
    return len - bytes_not_copied;
}

// ---------------- Initialization ----------------
static int __init mychardev_init(void)
{
    int ret;

    // 1️⃣ Allocate a device number dynamically
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    pr_info("mychardev: Major = %d, Minor = %d\n", MAJOR(dev_num), MINOR(dev_num));

    // 2️⃣ Initialize cdev structure and link it with file operations
    static struct file_operations fops = {
        .owner   = THIS_MODULE,
        .open    = my_open,
        .release = my_release,
        .read    = my_read,
        .write   = my_write
    };
    cdev_init(&my_cdev, &fops);

    // 3️⃣ Add device to system
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    // 4️⃣ Create class (for udev auto node creation)
    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class)) {
        pr_err("Failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    // 5️⃣ Create device node
    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("Failed to create device\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }

    // 6️⃣ Allocate kernel buffer
    kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!kernel_buffer) {
        pr_err("Failed to allocate memory\n");
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    pr_info("mychardev: Module loaded successfully\n");
    return 0;
}

// ---------------- Cleanup ----------------
static void __exit mychardev_exit(void)
{
    kfree(kernel_buffer);
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("mychardev: Module unloaded\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);

