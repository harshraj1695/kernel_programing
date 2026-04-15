// mymodule.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

static void do_heavy_work(void) {
    long i;
    for(i = 0; i < 10000000; i++){
        printk(KERN_DEBUG "Working... %ld\n", i);
        // sleep
        msleep(100);  // simulate slow work
    }  // simulate slow work
}

static int __init mymodule_init(void) {
    printk(KERN_INFO "Module loaded!\n");
    do_heavy_work();
    return 0;
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "Module removed!\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
MODULE_LICENSE("GPL");