// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>
#include <linux/string.h>

#include "virtusb_hub.h"

static unsigned int virtusb_hub_packed_word_index(unsigned int position,
                                                   unsigned int bits_per_value)
{
   return position / VIRTUSB_PACKED_VALUES_PER_WORD(bits_per_value);
}

static unsigned int virtusb_hub_packed_shift(unsigned int position,
                                              unsigned int bits_per_value)
{
   return (position % VIRTUSB_PACKED_VALUES_PER_WORD(bits_per_value)) * bits_per_value;
}

int virtusb_hub_init(struct virtusb_hub *hub, unsigned int port_count)
{
   if (hub == NULL) {
      return -EINVAL;
   }

   if ((port_count == 0U) || (port_count > VIRTUSB_MAX_HUB_PORTS)) {
      return -EINVAL;
   }

   memset(hub, 0, sizeof(*hub));

   hub->port_count = (u8)port_count;

   return 0;
}

void virtusb_hub_reset(struct virtusb_hub *hub)
{
   u8 port_count;

   if (hub == NULL) {
      return;
   }

   port_count = hub->port_count;

   memset(hub, 0, sizeof(*hub));

   hub->port_count = port_count;
}

bool virtusb_hub_port_is_valid(const struct virtusb_hub *hub,
                               unsigned int port_number)
{
   if (hub == NULL) {
      return false;
   }

   return (port_number > 0U) && (port_number <= hub->port_count);
}

enum virtusb_port_speed
virtusb_hub_get_port_speed(const struct virtusb_hub *hub,
                           unsigned int port_number)
{
   unsigned int word_index;
   unsigned int shift;
   u32 value;

   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return VIRTUSB_PORT_SPEED_NONE;
   }

   word_index = virtusb_hub_packed_word_index(port_number, VIRTUSB_PORT_SPEED_BITS);
   shift = virtusb_hub_packed_shift(port_number, VIRTUSB_PORT_SPEED_BITS);

   value = hub->usb.speed[word_index] >> shift;
   value &= VIRTUSB_PACKED_VALUE_MASK(VIRTUSB_PORT_SPEED_BITS);

   return (enum virtusb_port_speed)value;
}

int virtusb_hub_set_port_speed(struct virtusb_hub *hub,
                               unsigned int port_number,
                               enum virtusb_port_speed speed)
{
   unsigned int word_index;
   unsigned int shift;
   u32 mask;

   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return -EINVAL;
   }

   if ((unsigned int)speed > (unsigned int)VIRTUSB_PORT_SPEED_HIGH) {
      return -EINVAL;
   }

   word_index = virtusb_hub_packed_word_index(port_number, VIRTUSB_PORT_SPEED_BITS);
   shift = virtusb_hub_packed_shift(port_number, VIRTUSB_PORT_SPEED_BITS);

   mask = VIRTUSB_PACKED_VALUE_MASK(VIRTUSB_PORT_SPEED_BITS) << shift;

   hub->usb.speed[word_index] &= ~mask;
   hub->usb.speed[word_index] |= ((u32)speed << shift) & mask;

   return 0;
}
