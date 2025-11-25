#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>

struct pseudo_priv {
    void __iomem *base;
};

static int pseudo_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct pseudo_priv *priv;

    pr_info("PSEUDO_DRV: probe() CALLED!\n");

    /* Allocate private data */
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* Get memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENODEV;

    /* Map the memory */
    priv->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    pr_info("PSEUDO_DRV: mapped at %p\n", priv->base);

    platform_set_drvdata(pdev, priv);

    return 0;
}

static int pseudo_remove(struct platform_device *pdev)
{
    pr_info("PSEUDO_DRV: remove() CALLED!\n");
    return 0;
}

/* 🟢  DEVICE TREE MATCH TABLE  */
static const struct of_device_id pseudo_dt_ids[] = {
    { .compatible = "mycompany,pseudo" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pseudo_dt_ids);

/* 🟢 Platform driver */
static struct platform_driver pseudo_driver = {
    .probe  = pseudo_probe,
    .remove = pseudo_remove,
    .driver = {
        .name = "pseudo_dt_driver",
        .of_match_table = pseudo_dt_ids,   // <--- DT MATCHING HAPPENS HERE
    },
};

module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Minimal DT-based platform driver");

