#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Packet Monitor with Perf Visibility");

static struct nf_hook_ops nfho;
static unsigned long packet_count = 0;

/* Hook function */
unsigned int hook_func(void *priv,
                       struct sk_buff *skb,
                       const struct nf_hook_state *state)
{
    struct iphdr *ip_header;
    int i;

    if (!skb)
        return NF_ACCEPT;

    ip_header = ip_hdr(skb);
    if (!ip_header)
        return NF_ACCEPT;

    packet_count++;

    /* 🔥 Artificial workload */
    for (i = 0; i < 1000; i++) {
        cpu_relax();
    }

    return NF_ACCEPT;
}

/* Module init */
static int __init packet_monitor_init(void)
{
    printk(KERN_INFO "packet_monitor loaded\n");

    nfho.hook = hook_func;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &nfho);

    return 0;
}

/* Module exit */
static void __exit packet_monitor_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    printk(KERN_INFO "packet_monitor unloaded\n");
}

module_init(packet_monitor_init);
module_exit(packet_monitor_exit);