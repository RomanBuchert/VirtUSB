// SPDX-License-Identifier: GPL-2.0-only

#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "virtusb_hcd.h"

#define VIRTUSB_DEFAULT_HCD_COUNT 1U
#define VIRTUSB_MAX_HCD_COUNT     31U
#define VIRTUSB_ROOT_HUB_PORTS    31U

static unsigned int hcd_count = VIRTUSB_DEFAULT_HCD_COUNT;

module_param(hcd_count, uint, 0444);
MODULE_PARM_DESC(hcd_count, "Number of virtual USB host controllers (1..31)");

static struct platform_device **virtusb_hcd_devices;
static unsigned int virtusb_hcd_created_count;

static void virtusb_module_destroy_hcds(void)
{
   while (virtusb_hcd_created_count > 0U) {
      --virtusb_hcd_created_count;

      virtusb_hcd_destroy(virtusb_hcd_devices[virtusb_hcd_created_count]);
      virtusb_hcd_devices[virtusb_hcd_created_count] = NULL;
   }
}

static int __init virtusb_module_init(void)
{
   unsigned int instance;
   int ret;

   if ((hcd_count == 0U) || (hcd_count > VIRTUSB_MAX_HCD_COUNT)) {
      pr_err("VirtUSB: invalid hcd_count=%u, expected 1..%u\n",
             hcd_count,
             VIRTUSB_MAX_HCD_COUNT);

      return -EINVAL;
   }

   virtusb_hcd_devices = kcalloc(hcd_count,
                                 sizeof(*virtusb_hcd_devices),
                                 GFP_KERNEL);
   if (virtusb_hcd_devices == NULL) {
      return -ENOMEM;
   }

   ret = virtusb_hcd_driver_register();
   if (ret < 0) {
      pr_err("VirtUSB: failed to register HCD platform driver: %d\n", ret);
      goto free_hcd_devices;
   }

   for (instance = 0U; instance < hcd_count; ++instance) {
      virtusb_hcd_devices[instance] =
         virtusb_hcd_create(instance, VIRTUSB_ROOT_HUB_PORTS);

      if (IS_ERR(virtusb_hcd_devices[instance])) {
         ret = PTR_ERR(virtusb_hcd_devices[instance]);
         virtusb_hcd_devices[instance] = NULL;

         pr_err("VirtUSB: failed to create HCD instance %u: %d\n",
                instance,
                ret);

         goto destroy_hcds;
      }

      ++virtusb_hcd_created_count;
   }

   pr_info("VirtUSB module loaded with %u HCD instance(s)\n", hcd_count);

   return 0;

destroy_hcds:
   virtusb_module_destroy_hcds();
   virtusb_hcd_driver_unregister();

free_hcd_devices:
   kfree(virtusb_hcd_devices);
   virtusb_hcd_devices = NULL;

   return ret;
}

static void __exit virtusb_module_exit(void)
{
   virtusb_module_destroy_hcds();

   virtusb_hcd_driver_unregister();

   kfree(virtusb_hcd_devices);
   virtusb_hcd_devices = NULL;

   pr_info("VirtUSB module unloaded\n");
}

module_init(virtusb_module_init);
module_exit(virtusb_module_exit);

MODULE_AUTHOR("Roman Buchert");
MODULE_DESCRIPTION("Virtual USB host controller for Linux");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
