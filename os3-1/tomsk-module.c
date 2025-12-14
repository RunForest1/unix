#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

static int __init tomsk_init(void)
{
    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit tomsk_exit(void)
{
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tomsk_init);
module_exit(tomsk_exit);