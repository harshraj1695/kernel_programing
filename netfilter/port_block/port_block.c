#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>

static struct nf_hook_ops nfho;

/* Hook function */
static unsigned int udp_block_hook(void *priv,
                                   struct sk_buff *skb,
                                   const struct nf_hook_state *state)
{
    struct iphdr *ip;

    if (!skb)
        return NF_ACCEPT;

    ip = ip_hdr(skb);
    if (!ip)
        return NF_ACCEPT;

    // Block UDP packets
    if (ip->protocol == IPPROTO_TCP) {
        printk(KERN_INFO "Blocked UDP packet\n");
        return NF_DROP;
    }

    return NF_ACCEPT;
}

static int __init firewall_init(void)
{
    nfho.hook = udp_block_hook;
    nfho.hooknum = NF_INET_LOCAL_OUT;   // outgoing packets
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &nfho);

    printk(KERN_INFO "UDP firewall loaded\n");
    return 0;
}

/* Exit */
static void __exit firewall_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    printk(KERN_INFO "UDP firewall unloaded\n");
}

module_init(firewall_init);
module_exit(firewall_exit);

MODULE_LICENSE("GPL");