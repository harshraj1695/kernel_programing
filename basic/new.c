#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

static int  __init mymodule_init(void){
pr_info("hwllow kernel\n");
return 0;
}

static void __exit mymodule_exit(void){
pr_info("bye bye kernel\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HARSH RAJ SINGH");
MODULE_DESCRIPTION("KERNEL MODULE");


