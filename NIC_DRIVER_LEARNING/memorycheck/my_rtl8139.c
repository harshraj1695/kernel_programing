#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/delay.h>

#define VENDOR_ID 0x10ec
#define DEVICE_ID 0x8139

/* ── RTL8139 register offsets ───────────────────────────────── */
#define RTL_MAC         0x00
#define RTL_CMD         0x37
#define RTL_RBSTART     0x30
#define RTL_IMR         0x3C
#define RTL_ISR         0x3E
#define RTL_RCR         0x44
#define RTL_CAPR        0x38    /* Current Address of Packet Read */
#define RTL_CBR         0x3A    /* Current Buffer Address (chip write ptr) */

#define RTL_CMD_RST     0x10
#define RTL_CMD_RE      0x04
#define RTL_CMD_TE      0x08

#define RTL_ISR_ROK     0x0001  /* Receive OK   */
#define RTL_ISR_TOK     0x0004  /* Transmit OK  */
#define RTL_ISR_RER     0x0002  /* Receive Error */

/* RCR: accept broadcast + multicast + physical match, 8K buffer, no wrap */
#define RTL_RCR_VAL     0x0000C00F

/* ── Ring buffer config ─────────────────────────────────────── */
#define LOG_ENTRIES     16
#define LOG_MASK        (LOG_ENTRIES - 1)

#define ETH_HLEN        14
#define IP_PROTO_ICMP   1
#define ICMP_ECHO_REQ   8
#define ICMP_ECHO_REP   0

/* RX buffer size — 8K + 16 bytes overshoot guard + 1500 for WRAP=0 */
#define RX_BUF_SIZE     (8192 + 16 + 1500)

struct pkt_log_entry {
    ktime_t  ts;
    u16      pkt_len;
    u16      eth_type;
    u8       src_mac[6];
    u8       dst_mac[6];
    bool     is_icmp;
    u8       icmp_type;
    u8       icmp_code;
    u16      icmp_id;
    u16      icmp_seq;
    u8       ip_src[4];
    u8       ip_dst[4];
};

/* ── Private data ────────────────────────────────────────────── */
struct my_rtl8139_private {
    void __iomem        *ioaddr;
    void                *rx_buffer;
    dma_addr_t           rx_dma_handle;
    u32                  rx_buf_ptr;    /* our read pointer into rx ring */

    struct pkt_log_entry log[LOG_ENTRIES];
    unsigned int         log_head;
    unsigned int         log_count;
    spinlock_t           log_lock;
};

/* ── Packet parser ───────────────────────────────────────────── */
static void parse_and_log(struct my_rtl8139_private *priv,
                          const unsigned char *frame, u16 pkt_len)
{
    struct pkt_log_entry e = {};
    const unsigned char *ip, *icmp;
    unsigned int ip_hlen;
    unsigned long flags;

    e.ts       = ktime_get_real();
    e.pkt_len  = pkt_len;

    /* frame points directly at the Ethernet header (after RTL's 4-byte hdr) */
    memcpy(e.dst_mac, frame + 0, 6);
    memcpy(e.src_mac, frame + 6, 6);
    e.eth_type = (frame[12] << 8) | frame[13];

    if (e.eth_type == 0x0800 && pkt_len >= ETH_HLEN + 20) {
        ip      = frame + ETH_HLEN;
        ip_hlen = (ip[0] & 0x0f) * 4;

        if (ip[9] == IP_PROTO_ICMP &&
            pkt_len >= ETH_HLEN + ip_hlen + 8) {

            icmp        = ip + ip_hlen;
            e.is_icmp   = true;
            e.icmp_type = icmp[0];
            e.icmp_code = icmp[1];
            e.icmp_id   = (icmp[4] << 8) | icmp[5];
            e.icmp_seq  = (icmp[6] << 8) | icmp[7];
            memcpy(e.ip_src, ip + 12, 4);
            memcpy(e.ip_dst, ip + 16, 4);
        }
    }

    spin_lock_irqsave(&priv->log_lock, flags);
    priv->log[priv->log_head] = e;
    priv->log_head  = (priv->log_head + 1) & LOG_MASK;
    priv->log_count++;
    spin_unlock_irqrestore(&priv->log_lock, flags);
}

/* ── IRQ handler ─────────────────────────────────────────────── */
static irqreturn_t my_rtl8139_interrupt(int irq, void *dev_instance)
{
    struct my_rtl8139_private *priv = dev_instance;
    void __iomem *ioaddr = priv->ioaddr;
    u16 status;

    status = ioread16(ioaddr + RTL_ISR);
    if (status == 0 || status == 0xFFFF)
        return IRQ_NONE;

    /* Acknowledge immediately — QEMU needs this before we do anything */
    iowrite16(status, ioaddr + RTL_ISR);

    if (status & RTL_ISR_ROK) {
        /*
         * RTL8139 RX ring layout at rx_buf_ptr:
         *   [u16 pkt_status][u16 pkt_len][ethernet frame bytes...]
         *
         * rx_buf_ptr is our software read pointer, always kept
         * 4-byte aligned, wrapping within RX_BUF_SIZE.
         */
        while (1) {
            u8  *buf     = (u8 *)priv->rx_buffer;
            u32  offset  = priv->rx_buf_ptr % RX_BUF_SIZE;
            u16  rx_stat = le16_to_cpu(*(u16 *)(buf + offset));
            u16  rx_len  = le16_to_cpu(*(u16 *)(buf + offset + 2));
            u16  cbr;

            /* CBR is where the chip's DMA write pointer currently sits */
            cbr = ioread16(ioaddr + RTL_CBR);

            /* Stop if ring is empty (our ptr has caught up to chip ptr) */
            if ((priv->rx_buf_ptr % RX_BUF_SIZE) == cbr)
                break;

            /* Sanity check — bad packet or ring not ready yet */
            if (!(rx_stat & 0x0001) || rx_len < 8 || rx_len > 1536) {
                printk(KERN_WARNING "RTL8139: Bad RX stat=0x%04x len=%u, resetting ptr\n",
                       rx_stat, rx_len);
                priv->rx_buf_ptr = cbr;
                break;
            }

            parse_and_log(priv, buf + offset + 4, rx_len - 4);

            {
                struct pkt_log_entry *e =
                    &priv->log[(priv->log_head - 1) & LOG_MASK];

                if (e->is_icmp)
                    printk(KERN_INFO
                           "RTL8139: PING %s %pI4 -> %pI4 id=%u seq=%u\n",
                           e->icmp_type == ICMP_ECHO_REQ ? "REQ " : "REPL",
                           e->ip_src, e->ip_dst,
                           e->icmp_id, e->icmp_seq);
                else
                    printk(KERN_INFO
                           "RTL8139: RX len=%u ethertype=0x%04x src=%pM\n",
                           e->pkt_len, e->eth_type, e->src_mac);
            }

            /* Advance our read pointer: 4-byte header + packet, DWORD aligned */
            priv->rx_buf_ptr = (priv->rx_buf_ptr + 4 + rx_len + 3) & ~3;

            /*
             * Update CAPR — tell the chip how far we've read.
             * CAPR takes the offset minus 0x10 (hardware quirk).
             */
            iowrite16((u16)((priv->rx_buf_ptr % RX_BUF_SIZE) - 0x10),
                      ioaddr + RTL_CAPR);
        }
    }

    if (status & RTL_ISR_RER)
        printk(KERN_WARNING "RTL8139: RX error! ISR=0x%04x\n", status);

    return IRQ_HANDLED;
}

/* ── PCI ID table ────────────────────────────────────────────── */
static struct pci_device_id rtl8139_table[] = {
    { PCI_DEVICE(VENDOR_ID, DEVICE_ID) },
    { 0 }
};
MODULE_DEVICE_TABLE(pci, rtl8139_table);

/* ── Probe ───────────────────────────────────────────────────── */
static int my_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct my_rtl8139_private *priv;
    void __iomem *ioaddr;
    u8 mac[6];
    int i, err;

    printk(KERN_INFO "RTL8139: Probe started.\n");

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    spin_lock_init(&priv->log_lock);
    pci_set_drvdata(pdev, priv);

    err = pci_enable_device(pdev);
    if (err)
        return err;

    pci_set_master(pdev);

    err = pci_request_regions(pdev, "my_rtl8139");
    if (err) {
        printk(KERN_ERR "RTL8139: pci_request_regions failed\n");
        goto err_disable;
    }

    /* QEMU RTL8139: I/O registers are on BAR 0 */
    ioaddr = pci_iomap(pdev, 0, 0);
    if (!ioaddr) {
        printk(KERN_ERR "RTL8139: pci_iomap BAR0 failed\n");
        err = -ENOMEM;
        goto err_release;
    }
    priv->ioaddr = ioaddr;

    /* Read MAC */
    for (i = 0; i < 6; i++)
        mac[i] = ioread8(ioaddr + RTL_MAC + i);
    printk(KERN_INFO "RTL8139: MAC %pM\n", mac);

    /* Soft reset */
    iowrite8(RTL_CMD_RST, ioaddr + RTL_CMD);
    for (i = 0; i < 1000; i++) {
        if (!(ioread8(ioaddr + RTL_CMD) & RTL_CMD_RST))
            break;
        udelay(10);
    }
    printk(KERN_INFO "RTL8139: Reset complete.\n");

    /* Allocate DMA RX buffer */
    priv->rx_buffer = dma_alloc_coherent(&pdev->dev, RX_BUF_SIZE,
                                          &priv->rx_dma_handle, GFP_KERNEL);
    if (!priv->rx_buffer) {
        err = -ENOMEM;
        goto err_unmap;
    }

    priv->rx_buf_ptr = 0;

    /* Program RBSTART with the DMA physical address */
    iowrite32((u32)(priv->rx_dma_handle), ioaddr + RTL_RBSTART);

    /* Register IRQ */
    err = request_irq(pdev->irq, my_rtl8139_interrupt, IRQF_SHARED,
                      "my_rtl8139", priv);
    if (err) {
        printk(KERN_ERR "RTL8139: request_irq(%d) failed\n", pdev->irq);
        goto err_dma;
    }

    /* Unmask Receive OK interrupts */
    iowrite16(RTL_ISR_ROK | RTL_ISR_RER, ioaddr + RTL_IMR);

    /* RCR: accept broadcast/multicast/unicast, 8K ring, WRAP=1 */
    iowrite32(RTL_RCR_VAL, ioaddr + RTL_RCR);

    /* Enable RX + TX engines */
    iowrite8(RTL_CMD_RE | RTL_CMD_TE, ioaddr + RTL_CMD);

    printk(KERN_INFO "RTL8139: Ready on IRQ %d. Run: ping <guest-ip>\n",
           pdev->irq);
    return 0;

err_dma:
    dma_free_coherent(&pdev->dev, RX_BUF_SIZE,
                      priv->rx_buffer, priv->rx_dma_handle);
err_unmap:
    pci_iounmap(pdev, ioaddr);
err_release:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return err;
}

/* ── Remove — dump RAM log ───────────────────────────────────── */
static void my_remove(struct pci_dev *pdev)
{
    struct my_rtl8139_private *priv = pci_get_drvdata(pdev);
    unsigned int i, count, start;

    if (!priv)
        goto disable;

    /* Silence the hardware */
    iowrite8(0x00, priv->ioaddr + RTL_CMD);
    iowrite16(0x0000, priv->ioaddr + RTL_IMR);
    free_irq(pdev->irq, priv);

    /* ── Dump the in-RAM ring buffer ───────────────────────── */
    count = min(priv->log_count, (unsigned int)LOG_ENTRIES);
    printk(KERN_INFO "RTL8139: ===== Packet log (%u total, last %u) =====\n",
           priv->log_count, count);

    start = (priv->log_count >= LOG_ENTRIES) ? priv->log_head : 0;

    for (i = 0; i < count; i++) {
        struct pkt_log_entry *e = &priv->log[(start + i) & LOG_MASK];
        s64 sec  = ktime_to_ns(e->ts) / NSEC_PER_SEC;
        u32 usec = (u32)((ktime_to_ns(e->ts) % NSEC_PER_SEC) / 1000);

        if (e->is_icmp) {
            printk(KERN_INFO
                   "RTL8139: [%lld.%06u] #%u ICMP %-13s %pI4 -> %pI4"
                   " id=%u seq=%u len=%u src-mac=%pM\n",
                   sec, usec, priv->log_count - count + i,
                   e->icmp_type == ICMP_ECHO_REQ  ? "echo-request"  :
                   e->icmp_type == ICMP_ECHO_REP  ? "echo-reply"    : "other",
                   e->ip_src, e->ip_dst,
                   e->icmp_id, e->icmp_seq, e->pkt_len,
                   e->src_mac);
        } else {
            printk(KERN_INFO
                   "RTL8139: [%lld.%06u] #%u eth=0x%04x len=%u"
                   " src=%pM dst=%pM\n",
                   sec, usec, priv->log_count - count + i,
                   e->eth_type, e->pkt_len,
                   e->src_mac, e->dst_mac);
        }
    }
    printk(KERN_INFO "RTL8139: ===== End of log =====\n");

    dma_free_coherent(&pdev->dev, RX_BUF_SIZE,
                      priv->rx_buffer, priv->rx_dma_handle);
    pci_iounmap(pdev, priv->ioaddr);
    pci_release_regions(pdev);

disable:
    pci_disable_device(pdev);
    printk(KERN_INFO "RTL8139: Removed.\n");
}

/* ── Module wiring ───────────────────────────────────────────── */
static struct pci_driver my_driver = {
    .name     = "my_rtl8139_driver",
    .id_table = rtl8139_table,
    .probe    = my_probe,
    .remove   = my_remove,
};

module_pci_driver(my_driver);
MODULE_LICENSE("GPL");