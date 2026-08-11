// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>
#include <linux/string.h>

#include "virtusb_device.h"
#include "virtusb_hub.h"

static struct virtusb_hub *
virtusb_device_get_attached_hub(const struct virtusb_device *device,
                                unsigned int *port_number)
{
   struct virtusb_port *downstream;
   struct virtusb_hub *hub;

   if ((device == NULL) || (port_number == NULL)) {
      return NULL;
   }

   downstream = device->upstream_port.peer;
   if ((downstream == NULL) ||
       (downstream->role != VIRTUSB_PORT_ROLE_DOWNSTREAM)) {
      return NULL;
   }

   hub = downstream->owner;
   if (hub == NULL) {
      return NULL;
   }

   if (virtusb_hub_find_port_number(hub, downstream, port_number) < 0) {
      return NULL;
   }

   return hub;
}

int virtusb_device_init(struct virtusb_device *device, u8 speed)
{
   if (device == NULL) {
      return -EINVAL;
   }

   memset(device, 0, sizeof(*device));

   return virtusb_port_init(&device->upstream_port,
                            VIRTUSB_PORT_ROLE_UPSTREAM,
                            speed,
                            device);
}

int virtusb_device_attach(struct virtusb_device *device,
                          struct virtusb_port *downstream_port)
{
   struct virtusb_hub *hub;
   unsigned int port_number;
   bool was_connected;
   bool is_connected;
   int ret;

   if ((device == NULL) || (downstream_port == NULL)) {
      return -EINVAL;
   }

   hub = downstream_port->owner;
   if (hub == NULL) {
      return -EINVAL;
   }

   ret = virtusb_hub_find_port_number(hub, downstream_port, &port_number);
   if (ret < 0) {
      return ret;
   }

   was_connected = virtusb_hub_port_is_connected(hub, port_number);

   ret = virtusb_port_attach(downstream_port, &device->upstream_port);
   if (ret < 0) {
      return ret;
   }

   is_connected = virtusb_hub_port_is_connected(hub, port_number);
   if (was_connected != is_connected) {
      virtusb_hub_mark_port_connection_change(hub, port_number);
   }

   return 0;
}

void virtusb_device_detach(struct virtusb_device *device)
{
   struct virtusb_hub *hub;
   unsigned int port_number;
   bool was_connected;

   if (device == NULL) {
      return;
   }

   hub = virtusb_device_get_attached_hub(device, &port_number);
   if (hub == NULL) {
      virtusb_port_detach(&device->upstream_port);
      return;
   }

   was_connected = virtusb_hub_port_is_connected(hub, port_number);

   virtusb_port_detach(&device->upstream_port);

   if (was_connected) {
      virtusb_hub_mark_port_connection_change(hub, port_number);
   }
}

int virtusb_device_set_connection_signaling(struct virtusb_device *device,
                                             bool enabled)
{
   struct virtusb_hub *hub;
   unsigned int port_number;
   bool was_connected;
   bool is_connected;
   int ret;

   if (device == NULL) {
      return -EINVAL;
   }

   hub = virtusb_device_get_attached_hub(device, &port_number);
   if (hub == NULL) {
      return virtusb_port_set_connection_signaling(&device->upstream_port,
                                                    enabled);
   }

   was_connected = virtusb_hub_port_is_connected(hub, port_number);

   ret = virtusb_port_set_connection_signaling(&device->upstream_port, enabled);
   if (ret < 0) {
      return ret;
   }

   is_connected = virtusb_hub_port_is_connected(hub, port_number);
   if (was_connected != is_connected) {
      virtusb_hub_mark_port_connection_change(hub, port_number);
   }

   return 0;
}

bool virtusb_device_has_vbus(const struct virtusb_device *device)
{
   if (device == NULL) {
      return false;
   }

   return virtusb_port_has_vbus(&device->upstream_port);
}
