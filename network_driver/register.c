#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#define DRV_NAME "snull"

static struct net_device *snull_devs[2];  // Two virtual interfaces


 //Open function — called when 'ifconfig up' is run

static int snull_open(struct net_device *dev)
{
    netif_start_queue(dev);
    pr_info("%s: device opened\n", dev->name);
    return 0;
}


 //  Stop function — called when 'ifconfig down' is run
 
static int snull_stop(struct net_device *dev)
{
    netif_stop_queue(dev);
    pr_info("%s: device stopped\n", dev->name);
    return 0;
}


 //  Start Xmit — required placeholder for registration
 
static netdev_tx_t snull_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    pr_info("%s: dropping packet (dummy transmit)\n", dev->name);
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}


 //  net_device_ops structure

static const struct net_device_ops snull_netdev_ops = {
    .ndo_open       = snull_open,
    .ndo_stop       = snull_stop,
    .ndo_start_xmit = snull_start_xmit,
};


 //  Device Initialization (called for each sn0/sn1)
 
static void snull_setup(struct net_device *dev)
{
    ether_setup(dev);  // Fill in Ethernet defaults
    dev->netdev_ops = &snull_netdev_ops;
    dev->flags |= IFF_NOARP; // No ARP (simplified)
    strcpy(dev->name, "sn%d"); // Names: sn0, sn1
}


 //  Module Initialization — Register both interfaces
 
static int __init snull_init(void)
{
    int i, result;

    for (i = 0; i < 2; i++) {
        snull_devs[i] = alloc_netdev(0, "sn%d", NET_NAME_UNKNOWN, snull_setup);
        if (!snull_devs[i])
            return -ENOMEM;

        result = register_netdev(snull_devs[i]);
        if (result) {
            pr_err("snull: error %i registering device %s\n", result, snull_devs[i]->name);
            free_netdev(snull_devs[i]);
            return result;
        }
        pr_info("snull: registered device %s\n", snull_devs[i]->name);
    }

    pr_info("snull module loaded.\n");
    return 0;
}


 // Module Exit — Unregister both interfaces
 
static void __exit snull_cleanup(void)
{
    
    for (int i = 0; i < 2; i++) {
        if (snull_devs[i]) {
            unregister_netdev(snull_devs[i]);
            free_netdev(snull_devs[i]);
            pr_info("snull: unregistered device sn%d\n", i);
        }
    }
    pr_info("snull module unloaded.\n");
}

module_init(snull_init);
module_exit(snull_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Minimal snull-like network driver (registration only)");

