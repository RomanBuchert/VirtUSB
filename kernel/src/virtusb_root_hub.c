// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>

#include "virtusb_root_hub.h"

int virtusb_root_hub_init(struct virtusb_root_hub *root_hub,
                          unsigned int port_count)
{
   if (root_hub == NULL) {
      return -EINVAL;
   }

   return virtusb_hub_init(&root_hub->hub, port_count);
}

void virtusb_root_hub_reset(struct virtusb_root_hub *root_hub)
{
   if (root_hub == NULL) {
      return;
   }

   virtusb_hub_reset(&root_hub->hub);
}
