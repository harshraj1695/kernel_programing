#include <linux/module.h>
#include <linux/platform_device.h>

#define DEV_NAME "pseudo-device"

/* This is the memory area of your fake hardware */
static struct resource pseudo_resources[] = {
    {
        .start = 0x1000,
        .end   = 0x1000 + 0x10 - 1,
        .flags = IORESOURCE_MEM,
    },
};

/* Create a platform device object */
static struct platform_device pseudo_device = {
    .name = DEV_NAME,
    .id   = -1,                // only one instance
    .num_resources = ARRAY_SIZE(pseudo_resources),
    .resource = pseudo_resources,
};

/* Register device at module load */
static int __init pseudo_device_init(void)
{
    pr_info("PSEUDO_DEVICE: registering platform device\n");
    return platform_device_register(&pseudo_device);
}

/* Unregister device */
static void __exit pseudo_device_exit(void)
{
    pr_info("PSEUDO_DEVICE: unregistering platform device\n");
    platform_device_unregister(&pseudo_device);
}

module_init(pseudo_device_init);
module_exit(pseudo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Simple Pseudo Platform Device");

