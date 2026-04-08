#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>

static struct nf_hook_ops nfho;
static unsigned int firewall_hook(void *priv,
                                  struct sk_buff *skb,
                                  const struct nf_hook_state *state);

/* Change this IP to block */
#define BLOCKED_IP 0xC0A8010A  // 192.168.1.10 (hex)

unsigned int firewall_hook(void *priv,
                           struct sk_buff *skb,
                           const struct nf_hook_state *state)
{
    struct iphdr *ip;

    if (!skb)
        return NF_ACCEPT;

    ip = ip_hdr(skb);
    if (!ip)
        return NF_ACCEPT;

    /* Convert network → host byte order */
    if (ntohl(ip->saddr) == BLOCKED_IP) {
        printk(KERN_INFO "Blocked packet from %pI4\n", &ip->saddr);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

static int __init firewall_init(void)
{
    nfho.hook = firewall_hook;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &nfho);

    printk(KERN_INFO "Simple firewall loaded\n");
    return 0;
}

static void __exit firewall_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    printk(KERN_INFO "Simple firewall unloaded\n");
}

module_init(firewall_init);
module_exit(firewall_exit);

MODULE_LICENSE("GPL");