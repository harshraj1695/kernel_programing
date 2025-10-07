#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int myint = 0;
static char *mystring = "default";

module_param(myint, int, S_IRUGO);      
module_param(mystring, charp, S_IRUGO); 

MODULE_PARM_DESC(myint, "An integer parameter");
MODULE_PARM_DESC(mystring, "A string parameter");

static int __init my_init(void)
{
    pr_info("Module inserted with: myint=%d mystring=%s\n", myint, mystring);
    return 0;
}

static void __exit my_exit(void)
{
    pr_info("Module exiting\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

