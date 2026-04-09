#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/sysinfo.h>
#include <linux/ktime.h>

// proc file name
#define PROC_NAME "system_stats"

// counter
static unsigned long read_counter = 0;

// proc entry
static struct proc_dir_entry *proc_entry;

// show stats
static int proc_show(struct seq_file *m, void *v)
{
    struct sysinfo si;
    u64 uptime;

    // increment counter
    read_counter++;

    // memory info
    si_meminfo(&si);

    // uptime
    uptime = ktime_get_boottime_seconds();

    seq_printf(m, "=== Kernel System Stats ===\n\n");

    // uptime
    seq_printf(m, "Uptime: %llu seconds\n", uptime);

    // memory
    seq_printf(m, "Total RAM: %lu MB\n", (si.totalram * si.mem_unit) / (1024 * 1024));
    seq_printf(m, "Free RAM: %lu MB\n", (si.freeram * si.mem_unit) / (1024 * 1024));
    seq_printf(m, "Shared RAM: %lu MB\n", (si.sharedram * si.mem_unit) / (1024 * 1024));

    // counter
    seq_printf(m, "Read Counter: %lu\n", read_counter);

    return 0;
}

// open
static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

// proc ops
static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// init
static int __init proc_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0, NULL, &proc_fops);

    if (!proc_entry)
        return -ENOMEM;

    printk("system stats module loaded\n");
    return 0;
}

// exit
static void __exit proc_exit(void)
{
    proc_remove(proc_entry);
    printk("system stats module unloaded\n");
}

module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");