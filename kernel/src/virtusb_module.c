// SPDX-License-Identifier: GPL-2.0-only

#include <linux/init.h>
#include <linux/module.h>

static int __init virtusb_module_init(void)
{
	pr_info("VirtUSB module loaded\n");

	return 0;
}

static void __exit virtusb_module_exit(void)
{
	pr_info("VirtUSB module unloaded\n");
}

module_init(virtusb_module_init);
module_exit(virtusb_module_exit);

MODULE_AUTHOR("Roman Buchert");
MODULE_DESCRIPTION("Virtual USB host controller for Linux");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
