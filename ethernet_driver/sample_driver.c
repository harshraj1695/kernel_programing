#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/timer.h>

static struct net_device *dummy_dev;
static struct timer_list rx_timer;

/* ---------- fake RX ---------- */
static void dummy_rx(struct timer_list *t)
{
    struct sk_buff *skb;

    skb = netdev_alloc_skb(dummy_dev, 64);
    if (!skb)
        return;

    memcpy(skb_put(skb, 14), "HELLO_FROM_RX", 14);

    skb->protocol = eth_type_trans(skb, dummy_dev);
    netif_rx(skb);

    dummy_dev->stats.rx_packets++;
    dummy_dev->stats.rx_bytes += skb->len;

    mod_timer(&rx_timer, jiffies + HZ);
}

/* ---------- ndo_start_xmit ---------- */
static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *dev)
{
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;

    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static int dummy_open(struct net_device *dev)
{
    netif_start_queue(dev);
    mod_timer(&rx_timer, jiffies + HZ);
    return 0;
}

static int dummy_stop(struct net_device *dev)
{
    netif_stop_queue(dev);
    del_timer_sync(&rx_timer);
    return 0;
}

static const struct net_device_ops dummy_ops = {
    .ndo_open       = dummy_open,
    .ndo_stop       = dummy_stop,
    .ndo_start_xmit = dummy_xmit,
};

static int __init dummy_init(void)
{
    dummy_dev = alloc_etherdev(0);
    if (!dummy_dev)
        return -ENOMEM;

    strcpy(dummy_dev->name, "deth%d");
    dummy_dev->netdev_ops = &dummy_ops;

    eth_hw_addr_random(dummy_dev);

    timer_setup(&rx_timer, dummy_rx, 0);

    return register_netdev(dummy_dev);
}

static void __exit dummy_exit(void)
{
    unregister_netdev(dummy_dev);
    free_netdev(dummy_dev);
}

module_init(dummy_init);
module_exit(dummy_exit);

MODULE_LICENSE("GPL");

