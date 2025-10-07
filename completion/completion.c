#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/delay.h>

static struct task_struct *t;
static DECLARE_COMPLETION(done);

static int thread_fn(void *data)
{
    pr_info("Thread working...\n");
    msleep(2000);  // simulate work
    pr_info("Thread done!\n");
   // complete(&done); // signal completion
    return 0;
}

static int __init mymodule_init(void)
{
    pr_info("Module init\n");

    t = kthread_run(thread_fn, NULL, "mythread");
    if (IS_ERR(t)) return PTR_ERR(t);

    pr_info("Waiting for thread to complete...\n");
   // wait_for_completion(&done); // blocks here until thread signals
    pr_info("Thread completed, continuing in init\n");

    return 0;
}

static void __exit mymodule_exit(void)
{
    pr_info("Module exit\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Completion variable example");

