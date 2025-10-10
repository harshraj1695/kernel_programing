#include <linux/module.h>
#include <linux/interrupt.h>

static struct tasklet_struct my_tasklet;

void my_tasklet_function(unsigned long data)
{
    pr_info("Tasklet running on CPU %d, data=%lu\n", smp_processor_id(), data);
}

static int __init my_module_init(void)
{
    pr_info("Tasklet module loaded\n");

    tasklet_init(&my_tasklet, my_tasklet_function, 1234);
    tasklet_schedule(&my_tasklet);

    return 0;
}

static void __exit my_module_exit(void)
{
    tasklet_kill(&my_tasklet);
    pr_info("Tasklet module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");

