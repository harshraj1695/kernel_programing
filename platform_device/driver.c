#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#define DEV_NAME "pseudo-device"

struct pseudo_priv {
    void __iomem *base;
};

static int pseudo_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct pseudo_priv *priv;

    pr_info("PSEUDO_DRIVER: probe() called\n");

    /* Allocate storage for private data */
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* Fetch memory resource from platform_device */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        pr_err("PSEUDO_DRIVER: no mem resource\n");
        return -ENODEV;
    }

    /* Map memory */
    priv->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(priv->base)) {
        pr_err("PSEUDO_DRIVER: ioremap failed\n");
        return PTR_ERR(priv->base);
    }

    /* Save private data to device */
    platform_set_drvdata(pdev, priv);

    pr_info("PSEUDO_DRIVER: device mapped at %p\n", priv->base);
    return 0;
}

static int pseudo_remove(struct platform_device *pdev)
{
    pr_info("PSEUDO_DRIVER: remove() called\n");
    return 0;
}

/* Platform driver struct */
static struct platform_driver pseudo_driver = {
    .driver = {
        .name = DEV_NAME,    // MUST MATCH device name
    },
    .probe  = pseudo_probe,
    .remove = pseudo_remove,
};

module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Simple Pseudo Platform Driver");

