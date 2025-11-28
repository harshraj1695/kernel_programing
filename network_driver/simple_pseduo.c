// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/random.h>

#define DRV_NAME "pseudo_net"

// This is our fake device private data
struct pseudo_priv {
    struct net_device *ndev;
    struct timer_list rx_timer;   // Timer to simulate RX packets
};


//  TX FUNCTION (Called when kernel wants to send a packet)

static netdev_tx_t pseudo_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    pr_info("%s: Transmitting packet, length = %u bytes\n",
            dev->name, skb->len);

    // Fake: Free skb because we "sent" it
    dev_kfree_skb(skb);

    // Increase TX counter
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;

    return NETDEV_TX_OK;
}

// 
//  SIMULATE RECEIVE PACKETS (Timer-based RX interrupt)
// 
static void pseudo_rx_timer(struct timer_list *t)
{
    struct pseudo_priv *priv = from_timer(priv, t, rx_timer);
    struct net_device *dev = priv->ndev;

    // Allocate a fake incoming packet
    struct sk_buff *skb = dev_alloc_skb(64);
    if (!skb)
        return;

    // Add fake payload
    skb_put(skb, 64);
    memset(skb->data, 0xAB, 64);

    // Set protocol
    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);

    // Give the packet to upper layers
    pr_info("%s: Delivering fake RX packet to stack\n", dev->name);
    netif_rx(skb);

    dev->stats.rx_packets++;
    dev->stats.rx_bytes += 64;

    // Restart the timer
    mod_timer(&priv->rx_timer, jiffies + HZ); // 1 packet per sec
}


//  OPEN (Called when interface is brought up: ifconfig up)

static int pseudo_open(struct net_device *dev)
{
    struct pseudo_priv *priv = netdev_priv(dev);

    pr_info("%s: Device opened\n", dev->name);

    // Start RX simulation timer
    timer_setup(&priv->rx_timer, pseudo_rx_timer, 0);
    mod_timer(&priv->rx_timer, jiffies + HZ);

    netif_start_queue(dev);
    return 0;
}


//  STOP (Called when interface goes down)

static int pseudo_stop(struct net_device *dev)
{
    struct pseudo_priv *priv = netdev_priv(dev);

    pr_info("%s: Device stopped\n", dev->name);

    del_timer_sync(&priv->rx_timer);
    netif_stop_queue(dev);
    return 0;
}


//  SETUP FUNCTION (Like eth_setup)
// this will not work as dev_addr[] is read only
/*
static void pseudo_setup(struct net_device *dev)
{
    ether_setup(dev); // Fill ethernet defaults

    dev->netdev_ops = &(struct net_device_ops) {
        .ndo_open       = pseudo_open,
        .ndo_stop       = pseudo_stop,
        .ndo_start_xmit = pseudo_start_xmit,
    };

    // Fake MAC address
    dev->dev_addr[0] = 0x02;
    dev->dev_addr[1] = 0x00;
    dev->dev_addr[2] = 0x00;
    dev->dev_addr[3] = 0x00;
    dev->dev_addr[4] = 0x00;
    dev->dev_addr[5] = 0x01;
}   */

static void pseudo_setup(struct net_device *dev)
{
    ether_setup(dev); // Setup Ethernet defaults

    // our MAC address
    u8 fake_mac[ETH_ALEN] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

    eth_hw_addr_set(dev, fake_mac);

    dev->netdev_ops = &(struct net_device_ops) {
        .ndo_open = pseudo_open,
        .ndo_stop = pseudo_stop,
        .ndo_start_xmit = pseudo_start_xmit,
    };
}



//  MODULE INIT

static struct net_device *pseudo_dev;

static int __init pseudo_init(void)
{
    pr_info("Loading pseudo network driver\n");

    pseudo_dev = alloc_netdev(sizeof(struct pseudo_priv),
                              "pnet%d",
                              NET_NAME_UNKNOWN,
                              pseudo_setup);

    if (!pseudo_dev)
        return -ENOMEM;

    ((struct pseudo_priv *)netdev_priv(pseudo_dev))->ndev = pseudo_dev;

    return register_netdev(pseudo_dev);
}


//  MODULE EXIT

static void __exit pseudo_exit(void)
{
    pr_info("Unloading pseudo network driver\n");
    unregister_netdev(pseudo_dev);
    free_netdev(pseudo_dev);
}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Simple Pseudo Network Driver");

