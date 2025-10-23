#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>  // for tasklets

#define DEVICE_NAME "taskletdev"
#define MAGIC 'M'
#define CMD_PROCESS _IOWR(MAGIC, 1, int)

static dev_t dev;
static struct cdev my_cdev;
static struct class *cls;

// shared data (since tasklet cannot take parameters directly)
static int input_data;
static int result;
static DECLARE_TASKLET(my_tasklet, NULL); 

// Tasklet handler
static void tasklet_handler(unsigned long data)
{
    pr_info("Tasklet: received input=%d\n", input_data);

    // cannot sleep here — only do simple operations
    result = input_data * 2;

    pr_info("Tasklet: processed result=%d\n", result);
}


static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    if (cmd == CMD_PROCESS) {
        if (copy_from_user(&input_data, (int __user *)arg, sizeof(int)))
            return -EFAULT;

        pr_info("IOCTL: Got value %d from user\n", input_data);

        // Initialize the tasklet (attach handler)
        tasklet_init(&my_tasklet, tasklet_handler, 0);

        // Schedule the tasklet
        tasklet_schedule(&my_tasklet);

        // Wait until tasklet finishes
        tasklet_unlock_wait(&my_tasklet);

        // Send result back to user
        if (copy_to_user((int __user *)arg, &result, sizeof(int)))
            return -EFAULT;

        pr_info("IOCTL: Sent result %d back to user\n", result);
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = my_ioctl,
};

static int __init my_init(void)
{
    alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    cdev_init(&my_cdev, &fops);
    cdev_add(&my_cdev, dev, 1);
    cls = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(cls, NULL, dev, NULL, DEVICE_NAME);

    pr_info("Tasklet module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    tasklet_kill(&my_tasklet);  // make sure tasklet is not running
    device_destroy(cls, dev);
    class_destroy(cls);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Tasklet module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

/*Step 3: Explanation of the Changes
1. #include <linux/interrupt.h>

Tasklets are defined in this header file.

2. DECLARE_TASKLET(my_tasklet, NULL);

Declares a global tasklet variable.
Later we initialize it dynamically with tasklet_init() because we want to attach our custom handler.

3. tasklet_init(&my_tasklet, tasklet_handler, 0);

Initializes the tasklet with our handler function and an optional unsigned long data parameter (0 in this case).

4. tasklet_schedule(&my_tasklet);

Schedules the tasklet to run soon — not immediately.
The kernel adds it to the bottom half queue and executes it in softirq context.

5. tasklet_unlock_wait(&my_tasklet);

Ensures that the tasklet has finished before we proceed (acts like a wait).

6. tasklet_kill(&my_tasklet);

Used during module unload to stop any pending tasklet execution safely.

⚙️ When to Use Tasklet Over Workqueue

Use tasklets if:

You’re processing quick, non-blocking tasks.

You’re handling device events or interrupt data.

You don’t need to sleep or allocate large memory.

Use workqueues if:

You need to sleep (like doing I/O or memory allocation).

The work takes longer or involves user-space communication. */


