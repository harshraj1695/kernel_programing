#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>

struct student {
    int id;
    char name[20];
};

static int __init container_example_init(void)
{
    struct student s = { .id = 10, .name = "Harsh" };
    int *id_ptr = &s.id;

    struct student *ptr;

    // Use container_of() to get parent structure from member pointer
    ptr = container_of(id_ptr, struct student, id);

    printk(KERN_INFO "Original struct address: %p\n", &s);
    printk(KERN_INFO "Member pointer address : %p\n", id_ptr);
    printk(KERN_INFO "Recovered struct address: %p\n", ptr);
    printk(KERN_INFO "Student name = %s, id = %d\n", ptr->name, ptr->id);

    return 0;
}

static void __exit container_example_exit(void)
{
    printk(KERN_INFO "Container_of example module removed.\n");
}

module_init(container_example_init);
module_exit(container_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harsh Example");
MODULE_DESCRIPTION("Simple example of container_of() with normal variable");

