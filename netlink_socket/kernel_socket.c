#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#define NETLINK_USER 31

struct sock *nl_sk = NULL;

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size = 20;
    char reply_msg[] = "Hello from kernel";
    int res;

    pr_info("Netlink: Message received in kernel\n");

    /* Extract the received message */
    nlh = nlmsg_hdr(skb);
    pr_info("Received: %s\n", (char *)nlmsg_data(nlh));

    pid = nlh->nlmsg_pid; // PID of sending process

    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out)
        return;

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    strncpy(nlmsg_data(nlh), reply_msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0)
        pr_info("Error sending back to user\n");
}

static int __init simple_init(void)
{
    struct netlink_kernel_cfg cfg = {
        .groups = 0,
        .input = nl_recv_msg, // callback when userspace sends msg
    };

    pr_info("Simple Netlink example: Init\n");

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        pr_err("Error creating socket.\n");
        return -10;
    }

    return 0;
}

static void __exit simple_exit(void)
{
    pr_info("Simple Netlink example: Exit\n");
    netlink_kernel_release(nl_sk);
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");

