#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/timekeeping.h>

#define PROCFS_NAME "bttf_time"

static struct proc_dir_entry *proc_file;

// Timestamp для 5 ноября 1955 года
static time64_t back_to_future_timestamp(void)
{
    // 5 ноября 1955 года
    return mktime64(1955, 11, 5, 0, 0, 0);
}

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer, size_t buffer_length, loff_t *offset)
{
    char msg[256];
    int len;
    time64_t now = ktime_get_real_seconds();
    time64_t past = back_to_future_timestamp();
    time64_t diff = now - past;

    // Расчет времени в разных единицах
    unsigned long years = diff / (365 * 24 * 3600);
    unsigned long days = (diff % (365 * 24 * 3600)) / (24 * 3600);
    unsigned long hours = (diff % (24 * 3600)) / 3600;
    unsigned long minutes = (diff % 3600) / 60;
    unsigned long seconds = diff % 60;

    if (*offset > 0)
        return 0;

    len = snprintf(msg, sizeof(msg),
                   "Time since Nov 5, 1955 (Back to the Future):\n"
                   "%lu years, %lu days, %lu hours, %lu minutes, %lu seconds\n"
                   "Total seconds: %lld\n",
                   years, days, hours, minutes, seconds, diff);

    if (copy_to_user(buffer, msg, len))
        return -EFAULT;

    *offset = len;
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init backinfutures_init(void)
{
    printk(KERN_INFO "Welcome to the Tomsk State University\n");

    proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_file_fops);
    if (!proc_file)
    {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    return 0;
}

static void __exit backinfutures_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(backinfutures_init);
module_exit(backinfutures_exit);

MODULE_LICENSE("GPL");