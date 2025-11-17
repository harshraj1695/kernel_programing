#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *task;

int my_kthread(void *data)
{
    while (!kthread_should_stop()) {
        pr_info("Running on CPU %d\n", smp_processor_id());
        msleep(1000);
    }
    return 0;
}

static int __init my_init(void)
{
    task = kthread_create(my_kthread, NULL, "bound_thread");
    if (IS_ERR(task))
        return PTR_ERR(task);

    kthread_bind(task, 1);   
    wake_up_process(task);
    return 0;
}

static void __exit my_exit(void)
{
    kthread_stop(task);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

