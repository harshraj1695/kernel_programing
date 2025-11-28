// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/errno.h>

#define DRV_NAME "simple_pseudo"

struct pseudo_priv {
    struct net_device *ndev;
    struct timer_list rx_timer;
};

/* Forward declarations */
static int pseudo_open(struct net_device *dev);
static int pseudo_stop(struct net_device *dev);
static netdev_tx_t pseudo_start_xmit(struct sk_buff *skb, struct net_device *dev);
static void pseudo_rx_timer(struct timer_list *t);

/* --- netdev ops must be a static (not temporary) object --- */
static const struct net_device_ops pseudo_netdev_ops = {
    .ndo_open       = pseudo_open,
    .ndo_stop       = pseudo_stop,
    .ndo_start_xmit = pseudo_start_xmit,
};

/* TX: accept skb, update stats, free it */
static netdev_tx_t pseudo_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    /* update stats */
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;

    pr_info("%s: pseudo_start_xmit len=%u\n", dev->name, skb->len);

    /* pretend to send, drop packet */
    dev_kfree_skb(skb);

    return NETDEV_TX_OK;
}

/* Timer callback: simulate a received packet */
static void pseudo_rx_timer(struct timer_list *t)
{
    struct pseudo_priv *priv = from_timer(priv, t, rx_timer);
    struct net_device *dev = priv->ndev;
    struct sk_buff *skb;
    unsigned int pkt_len = 64;

    if (!dev)
        return;

    skb = netdev_alloc_skb(dev, pkt_len + NET_IP_ALIGN);
    if (!skb)
        goto out;

    skb_reserve(skb, NET_IP_ALIGN); /* align */
    skb_put(skb, pkt_len);
    memset(skb->data, 0xAB, pkt_len);

    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;

    /* update stats */
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += pkt_len;

    pr_info("%s: delivering fake RX packet\n", dev->name);
    netif_rx(skb);

out:
    /* re-arm timer (deliver one packet per second while up) */
    if (netif_running(dev))
        mod_timer(&priv->rx_timer, jiffies + HZ);
}

/* open: start queue and timer */
static int pseudo_open(struct net_device *dev)
{
    struct pseudo_priv *priv = netdev_priv(dev);

    pr_info("%s: open\n", dev->name);

    /* setup timer here (not in init) to avoid referencing uninitialized data */
    timer_setup(&priv->rx_timer, pseudo_rx_timer, 0);
    mod_timer(&priv->rx_timer, jiffies + HZ);

    netif_start_queue(dev);
    return 0;
}

/* stop: stop queue and delete timer */
static int pseudo_stop(struct net_device *dev)
{
    struct pseudo_priv *priv = netdev_priv(dev);

    pr_info("%s: stop\n", dev->name);

    netif_stop_queue(dev);
    del_timer_sync(&priv->rx_timer);
    return 0;
}

/* setup: called by alloc_netdev; configure device and link to ops */
static void pseudo_setup(struct net_device *dev)
{
    ether_setup(dev);

    /* avoid assigning dev->dev_addr[] directly; use helper */
    {
        u8 fake_mac[ETH_ALEN] = {0x02,0x00,0x00,0x00,0x00,0x01};
        eth_hw_addr_set(dev, fake_mac);
    }

    dev->netdev_ops = &pseudo_netdev_ops;

    dev->min_mtu = 68;
    dev->max_mtu = 1500;
}

/* global pointer so exit can access it */
static struct net_device *pseudo_dev;

static int __init pseudo_init(void)
{
    int ret;

    pr_info("%s: init\n", DRV_NAME);

    /* allocate netdev and private area */
    pseudo_dev = alloc_netdev(sizeof(struct pseudo_priv), "pnet%d",
                              NET_NAME_UNKNOWN, pseudo_setup);
    if (!pseudo_dev) {
        pr_err("%s: alloc_netdev failed\n", DRV_NAME);
        return -ENOMEM;
    }

    /* save ndev pointer into priv so timer callback can find dev */
    ((struct pseudo_priv *)netdev_priv(pseudo_dev))->ndev = pseudo_dev;

    ret = register_netdev(pseudo_dev);
    if (ret) {
        pr_err("%s: register_netdev failed: %d\n", DRV_NAME, ret);
        free_netdev(pseudo_dev);
        pseudo_dev = NULL;
        return ret;
    }

    pr_info("%s: registered %s\n", DRV_NAME, pseudo_dev->name);
    return 0;
}

static void __exit pseudo_exit(void)
{
    pr_info("%s: exit\n", DRV_NAME);

    if (!pseudo_dev)
        return;

    unregister_netdev(pseudo_dev);
    free_netdev(pseudo_dev);
    pseudo_dev = NULL;
}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Simple pseudo network driver - fixed");

