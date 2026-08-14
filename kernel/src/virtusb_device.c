// SPDX-License-Identifier: GPL-2.0-only

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "virtusb_device.h"
#include "virtusb_hub.h"
#include "virtusb_object_manager.h"

static void virtusb_device_release(struct virtusb_object *object)
{
   struct virtusb_device *device;

   device = container_of(object, struct virtusb_device, object);

   WARN_ON(device->upstream_port.peer != NULL);

   kfree(device);
}

struct virtusb_device *virtusb_device_create(u8 speed)
{
   struct virtusb_object *object;
   struct virtusb_device *device;
   int ret;

   object = virtusb_object_alloc(sizeof(*device), virtusb_device_release);
   if (object == NULL) {
      return ERR_PTR(-ENOMEM);
   }

   device = container_of(object, struct virtusb_device, object);

   ret = virtusb_port_init(&device->upstream_port,
                           VIRTUSB_PORT_ROLE_UPSTREAM,
                           speed,
                           device);
   if (ret < 0) {
      virtusb_object_put(object);
      return ERR_PTR(ret);
   }

   ret = virtusb_object_register(object, VIRTUSB_OBJECT_TYPE_DEVICE);
   if (ret < 0) {
      virtusb_object_put(object);
      return ERR_PTR(ret);
   }

   return device;
}

int virtusb_device_destroy(struct virtusb_device *device, bool force)
{
   int ret;

   if (device == NULL) {
      return -EINVAL;
   }

   if (device->upstream_port.peer != NULL) {
      if (!force) {
         return -EBUSY;
      }

      ret = virtusb_device_set_connection_signaling(device, false);
      if (ret < 0) {
         return ret;
      }

      virtusb_device_detach(device);
   }

   return virtusb_object_unregister(&device->object);
}

void virtusb_device_shutdown_all(void)
{
   struct virtusb_object *object;
   struct virtusb_device *device;

   for (;;) {
      object = virtusb_object_lookup_first_by_type(VIRTUSB_OBJECT_TYPE_DEVICE);
      if (object == NULL) {
         return;
      }

      device = container_of(object, struct virtusb_device, object);

      (void)virtusb_device_destroy(device, true);
      virtusb_object_put(object);
   }
}

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
