#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

struct my_work_data {
    struct work_struct work;
    int value;
};

static struct workqueue_struct *my_wq;

void work_handler(struct work_struct *work)
{
    struct my_work_data *data = container_of(work, struct my_work_data, work);
    printk(KERN_INFO "Work running, value=%d\n", data->value);
    kfree(data);
}

static int __init my_init(void)
{
    my_wq = create_singlethread_workqueue("my_wq");

    struct my_work_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
    data->value = 123;
    INIT_WORK(&data->work, work_handler);
    queue_work(my_wq, &data->work);

    return 0;
}

static void __exit my_exit(void)
{
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

