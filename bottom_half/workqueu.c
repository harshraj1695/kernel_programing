#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

static struct workqueue_struct *my_wq;
static struct work_struct my_work;

static void my_work_function(struct work_struct *work)
{
    pr_info("Workqueue: running in process context on CPU %d\n", smp_processor_id());
    ssleep(2);  // safe: we can sleep here
}

static int __init my_init(void)
{
    my_wq = alloc_workqueue("my_wq", WQ_UNBOUND, 0);
    INIT_WORK(&my_work, my_work_function);
    queue_work(my_wq, &my_work);
    pr_info("Workqueue module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    flush_workqueue(my_wq);  // wait for work to finish
    destroy_workqueue(my_wq);
    pr_info("Workqueue module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

