#include <linux/init.h>
#include <linux/module.h>

static int __init my_kernel_module_init(void)
{
    printk(KERN_INFO "Hello, kernel!\n");
    return 0;
}

static void __exit my_kernel_module_exit(void)
{
    printk(KERN_INFO "Goodbye, kernel!\n");
}

module_init(my_kernel_module_init);
module_exit(my_kernel_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John");
MODULE_DESCRIPTION("A simple kernel module");