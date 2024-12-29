#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Hello World du Kernel");
MODULE_AUTHOR("Aiden");
MODULE_LICENSE("GLP");

static int Message_init(void) {
	printk("Hello world!\n");
	return 0;
}

static void Message_exit(void) {
	printk("Goodbye world\n");
}

module_init(Message_init);
module_exit(Message_exit);
