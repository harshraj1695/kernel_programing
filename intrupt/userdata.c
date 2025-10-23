#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#define DEVICE_NAME "wqdev"
#define MAGIC 'M'
#define CMD_PROCESS _IOWR(MAGIC, 1, int)

static dev_t dev;
static struct cdev my_cdev;
static struct class *cls;

struct my_work_data {
    struct work_struct work;
    int input;
    int *result_ptr;  // store address to send result back
};

static struct workqueue_struct *my_wq;

static void work_handler(struct work_struct *work)
{
    struct my_work_data *data = container_of(work, struct my_work_data, work);

    pr_info("Workqueue: received input=%d\n", data->input);


    *(data->result_ptr) = data->input * 2;

    pr_info("Workqueue: processed result=%d\n", *(data->result_ptr));

    kfree(data);  
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int user_val, result = 0;

    if (cmd == CMD_PROCESS) {

        if (copy_from_user(&user_val, (int __user *)arg, sizeof(int)))
            return -EFAULT;

        pr_info("IOCTL: Got value %d from user\n", user_val);


        struct my_work_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
        if (!data)
            return -ENOMEM;

        data->input = user_val;
        data->result_ptr = &result;

        INIT_WORK(&data->work, work_handler);

        // queue the work
        queue_work(my_wq, &data->work);

        // wait until work is finished
        flush_workqueue(my_wq);

        // send result back to user
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

    my_wq = create_singlethread_workqueue("my_workqueue");

    pr_info("Module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    device_destroy(cls, dev);
    class_destroy(cls);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

