// SPDX-License-Identifier: GPL-2.0-only

#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "virtusb_hub.h"

static void virtusb_hub_notify_status_changed(struct virtusb_hub *hub)
{
   if ((hub != NULL) && (hub->status_changed != NULL)) {
      hub->status_changed(hub, hub->status_changed_context);
   }
}

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

static void virtusb_hub_set_port_speed(struct virtusb_hub *hub,
                                       unsigned int port_number,
                                       enum virtusb_port_speed speed)
{
   unsigned int word_index;
   unsigned int shift;
   u32 mask;

   word_index = virtusb_hub_packed_word_index(port_number, VIRTUSB_PORT_SPEED_BITS);
   shift = virtusb_hub_packed_shift(port_number, VIRTUSB_PORT_SPEED_BITS);
   mask = VIRTUSB_PACKED_VALUE_MASK(VIRTUSB_PORT_SPEED_BITS) << shift;

   hub->usb.speed[word_index] &= ~mask;
   hub->usb.speed[word_index] |= ((u32)speed << shift) & mask;
}

static enum virtusb_port_speed
virtusb_hub_determine_port_speed(const struct virtusb_hub *hub,
                                 unsigned int port_number)
{
   const struct virtusb_port *downstream;
   const struct virtusb_port *upstream;
   u8 capabilities;

   downstream = virtusb_hub_get_port_const(hub, port_number);
   if (downstream == NULL) {
      return VIRTUSB_PORT_SPEED_NONE;
   }

   upstream = downstream->peer;
   if ((upstream == NULL) ||
       (upstream->role != VIRTUSB_PORT_ROLE_UPSTREAM)) {
      return VIRTUSB_PORT_SPEED_NONE;
   }

   capabilities = downstream->speed & upstream->speed;

   if ((capabilities & VIRTUSB_PORT_SPEED_CAP_HIGH) != 0U) {
      return VIRTUSB_PORT_SPEED_HIGH;
   }

   if ((capabilities & VIRTUSB_PORT_SPEED_CAP_FULL) != 0U) {
      return VIRTUSB_PORT_SPEED_FULL;
   }

   if ((capabilities & VIRTUSB_PORT_SPEED_CAP_LOW) != 0U) {
      return VIRTUSB_PORT_SPEED_LOW;
   }

   return VIRTUSB_PORT_SPEED_NONE;
}

int virtusb_hub_init(struct virtusb_hub *hub, unsigned int port_count)
{
   unsigned int port;
   int ret;

   if (hub == NULL) {
      return -EINVAL;
   }

   if ((port_count == 0U) || (port_count > VIRTUSB_MAX_HUB_PORTS)) {
      return -EINVAL;
   }

   memset(hub, 0, sizeof(*hub));

   hub->port_count = (u8)port_count;
   hub->power_switching_mode = VIRTUSB_HUB_POWER_SWITCHING_INDIVIDUAL;
   hub->over_current_mode = VIRTUSB_HUB_OVER_CURRENT_PER_PORT;

   for (port = 0U; port < port_count; ++port) {
      ret = virtusb_port_init(&hub->ports[port],
                              VIRTUSB_PORT_ROLE_DOWNSTREAM,
                              (u8)VIRTUSB_PORT_SPEED_CAP_ALL,
                              hub);
      if (ret < 0) {
         memset(hub, 0, sizeof(*hub));
         return ret;
      }
   }

   return 0;
}

void virtusb_hub_reset(struct virtusb_hub *hub)
{
   if (hub == NULL) {
      return;
   }

   /*
    * A hub protocol reset must not alter physical topology or the simulated
    * downstream hardware state. Only USB protocol state is cleared here.
    */
   memset(&hub->usb, 0, sizeof(hub->usb));
   memset(&hub->usb_change, 0, sizeof(hub->usb_change));
}

void virtusb_hub_set_status_changed_callback(
   struct virtusb_hub *hub,
   virtusb_hub_status_changed_t callback,
   void *context)
{
   if (hub == NULL) {
      return;
   }

   hub->status_changed = callback;
   hub->status_changed_context = context;
}

bool virtusb_hub_port_is_valid(const struct virtusb_hub *hub,
                               unsigned int port_number)
{
   if (hub == NULL) {
      return false;
   }

   return (port_number > 0U) && (port_number <= hub->port_count);
}

struct virtusb_port *virtusb_hub_get_port(struct virtusb_hub *hub,
                                          unsigned int port_number)
{
   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return NULL;
   }

   return &hub->ports[port_number - 1U];
}

const struct virtusb_port *
virtusb_hub_get_port_const(const struct virtusb_hub *hub,
                           unsigned int port_number)
{
   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return NULL;
   }

   return &hub->ports[port_number - 1U];
}

int virtusb_hub_find_port_number(const struct virtusb_hub *hub,
                                 const struct virtusb_port *port,
                                 unsigned int *port_number)
{
   unsigned int index;

   if ((hub == NULL) || (port == NULL) || (port_number == NULL)) {
      return -EINVAL;
   }

   for (index = 0U; index < hub->port_count; ++index) {
      if (&hub->ports[index] == port) {
         *port_number = index + 1U;
         return 0;
      }
   }

   return -ENOENT;
}

void virtusb_hub_mark_port_connection_change(struct virtusb_hub *hub,
                                              unsigned int port_number)
{
   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return;
   }

   if (!virtusb_hub_port_is_connected(hub, port_number)) {
      hub->usb.enabled &= ~BIT(port_number);
      hub->usb.suspended &= ~BIT(port_number);
      hub->usb.reset &= ~BIT(port_number);
      virtusb_hub_set_port_speed(hub, port_number, VIRTUSB_PORT_SPEED_NONE);
   }

   if ((hub->usb_change.connected & BIT(port_number)) != 0U) {
      return;
   }

   hub->usb_change.connected |= BIT(port_number);
   virtusb_hub_notify_status_changed(hub);
}

int virtusb_hub_reset_port(struct virtusb_hub *hub,
                           unsigned int port_number)
{
   enum virtusb_port_speed speed;

   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return -EINVAL;
   }

   if (!virtusb_hub_port_is_connected(hub, port_number)) {
      return -ENODEV;
   }

   speed = virtusb_hub_determine_port_speed(hub, port_number);
   if (speed == VIRTUSB_PORT_SPEED_NONE) {
      return -EOPNOTSUPP;
   }

   /*
    * The protocol state models the result of a real USB reset sequence. Reset
    * timing is intentionally not simulated yet; completion is synchronous.
    */
   hub->usb.enabled &= ~BIT(port_number);
   hub->usb.suspended &= ~BIT(port_number);
   hub->usb.reset |= BIT(port_number);

   virtusb_hub_set_port_speed(hub, port_number, speed);

   hub->usb.reset &= ~BIT(port_number);
   hub->usb.enabled |= BIT(port_number);

   if ((hub->usb_change.reset & BIT(port_number)) == 0U) {
      hub->usb_change.reset |= BIT(port_number);
      virtusb_hub_notify_status_changed(hub);
   }

   return 0;
}

int virtusb_hub_set_power_switching_mode(
   struct virtusb_hub *hub,
   enum virtusb_hub_power_switching_mode mode)
{
   if (hub == NULL) {
      return -EINVAL;
   }

   if ((mode != VIRTUSB_HUB_POWER_SWITCHING_GANGED) &&
       (mode != VIRTUSB_HUB_POWER_SWITCHING_INDIVIDUAL)) {
      return -EINVAL;
   }

   hub->power_switching_mode = mode;

   return 0;
}

int virtusb_hub_set_over_current_mode(
   struct virtusb_hub *hub,
   enum virtusb_hub_over_current_mode mode)
{
   if (hub == NULL) {
      return -EINVAL;
   }

   if ((mode != VIRTUSB_HUB_OVER_CURRENT_GLOBAL) &&
       (mode != VIRTUSB_HUB_OVER_CURRENT_PER_PORT) &&
       (mode != VIRTUSB_HUB_OVER_CURRENT_NONE)) {
      return -EINVAL;
   }

   hub->over_current_mode = mode;

   return 0;
}

int virtusb_hub_set_port_power(struct virtusb_hub *hub,
                               unsigned int port_number,
                               bool powered)
{
   struct virtusb_port *port;
   bool was_connected;
   bool is_connected;
   int ret;

   port = virtusb_hub_get_port(hub, port_number);
   if (port == NULL) {
      return -EINVAL;
   }

   if (hub->power_switching_mode == VIRTUSB_HUB_POWER_SWITCHING_GANGED) {
      return virtusb_hub_set_all_ports_power(hub, powered);
   }

   was_connected = virtusb_hub_port_is_connected(hub, port_number);

   ret = virtusb_port_set_powered(port, powered);
   if (ret < 0) {
      return ret;
   }

   is_connected = virtusb_hub_port_is_connected(hub, port_number);
   if (was_connected != is_connected) {
      virtusb_hub_mark_port_connection_change(hub, port_number);
   }

   return 0;
}

int virtusb_hub_set_all_ports_power(struct virtusb_hub *hub, bool powered)
{
   unsigned int port_number;
   bool was_connected;
   bool is_connected;
   int ret;

   if (hub == NULL) {
      return -EINVAL;
   }

   for (port_number = 1U; port_number <= hub->port_count; ++port_number) {
      was_connected = virtusb_hub_port_is_connected(hub, port_number);

      ret = virtusb_port_set_powered(&hub->ports[port_number - 1U], powered);
      if (ret < 0) {
         return ret;
      }

      is_connected = virtusb_hub_port_is_connected(hub, port_number);
      if (was_connected != is_connected) {
         virtusb_hub_mark_port_connection_change(hub, port_number);
      }
   }

   return 0;
}

bool virtusb_hub_port_is_powered(const struct virtusb_hub *hub,
                                 unsigned int port_number)
{
   const struct virtusb_port *port;

   port = virtusb_hub_get_port_const(hub, port_number);
   if (port == NULL) {
      return false;
   }

   return port->state.downstream.powered;
}

int virtusb_hub_set_port_over_current(struct virtusb_hub *hub,
                                      unsigned int port_number,
                                      bool over_current)
{
   struct virtusb_port *port;

   if ((hub == NULL) ||
       (hub->over_current_mode != VIRTUSB_HUB_OVER_CURRENT_PER_PORT)) {
      return -EINVAL;
   }

   port = virtusb_hub_get_port(hub, port_number);
   if (port == NULL) {
      return -EINVAL;
   }

   return virtusb_port_set_over_current(port, over_current);
}

bool virtusb_hub_port_is_over_current(const struct virtusb_hub *hub,
                                      unsigned int port_number)
{
   const struct virtusb_port *port;

   if ((hub == NULL) ||
       (hub->over_current_mode != VIRTUSB_HUB_OVER_CURRENT_PER_PORT)) {
      return false;
   }

   port = virtusb_hub_get_port_const(hub, port_number);
   if (port == NULL) {
      return false;
   }

   return port->state.downstream.over_current;
}

void virtusb_hub_set_over_current(struct virtusb_hub *hub, bool over_current)
{
   if ((hub == NULL) ||
       (hub->over_current_mode != VIRTUSB_HUB_OVER_CURRENT_GLOBAL)) {
      return;
   }

   hub->over_current = over_current;
}

bool virtusb_hub_is_over_current(const struct virtusb_hub *hub)
{
   if ((hub == NULL) ||
       (hub->over_current_mode != VIRTUSB_HUB_OVER_CURRENT_GLOBAL)) {
      return false;
   }

   return hub->over_current;
}

bool virtusb_hub_port_is_connected(const struct virtusb_hub *hub,
                                   unsigned int port_number)
{
   const struct virtusb_port *downstream;
   const struct virtusb_port *upstream;

   downstream = virtusb_hub_get_port_const(hub, port_number);
   if (downstream == NULL) {
      return false;
   }

   upstream = downstream->peer;
   if ((upstream == NULL) || (upstream->role != VIRTUSB_PORT_ROLE_UPSTREAM)) {
      return false;
   }

   if (!downstream->state.downstream.powered) {
      return false;
   }

   return upstream->state.upstream.connection_signaling;
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
