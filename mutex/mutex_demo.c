/* mutex_demo.c */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Simple mutex use case in kernel module");

static struct task_struct *t1, *t2;
static int shared_counter = 0;
static struct mutex my_lock;

static int worker_fn(void *data)
{
    const char *name = data;
    int i;

    for (i = 0; i < 5 && !kthread_should_stop(); i++) {
        mutex_lock(&my_lock);

        shared_counter++;
        pr_info("%s incremented counter -> %d\n", name, shared_counter);

        /* Simulate some work */
        msleep(100);

        mutex_unlock(&my_lock);

        msleep(50);
    }
    pr_info("%s: exiting\n", name);
    return 0;
}

static int __init mutex_demo_init(void)
{
    int ret;

    pr_info("mutex_demo: init\n");
    mutex_init(&my_lock);

    /* Start thread1 */
    t1 = kthread_run(worker_fn, "Thread1", "thread1");
    if (IS_ERR(t1)) {
        ret = PTR_ERR(t1);
        pr_err("mutex_demo: failed to start thread1 (%d)\n", ret);
        t1 = NULL;
        return ret;
    }

    /* Start thread2 */
    t2 = kthread_run(worker_fn, "Thread2", "thread2");
    if (IS_ERR(t2)) {
        ret = PTR_ERR(t2);
        pr_err("mutex_demo: failed to start thread2 (%d)\n", ret);
        t2 = NULL;
        kthread_stop(t1);  // cleanup thread1 if thread2 fails
        t1 = NULL;
        return ret;
    }

    return 0;
}

static void __exit mutex_demo_exit(void)
{
    pr_info("mutex_demo: exit\n");

    if (t1) {
        kthread_stop(t1);
        t1 = NULL;
    }

    if (t2) {
        kthread_stop(t2);
        t2 = NULL;
    }

    pr_info("mutex_demo: final counter = %d\n", shared_counter);
}

module_init(mutex_demo_init);
module_exit(mutex_demo_exit);

