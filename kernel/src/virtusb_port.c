// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>
#include <linux/string.h>

#include "virtusb_port.h"

static bool virtusb_port_speed_is_valid(u8 speed)
{
   if (speed == 0U) {
      return false;
   }

   return (speed & (u8)~VIRTUSB_PORT_SPEED_CAP_ALL) == 0U;
}

int virtusb_port_init(struct virtusb_port *port,
                      enum virtusb_port_role role,
                      u8 speed,
                      void *owner)
{
   if ((port == NULL) || (owner == NULL)) {
      return -EINVAL;
   }

   if ((role != VIRTUSB_PORT_ROLE_UPSTREAM) &&
       (role != VIRTUSB_PORT_ROLE_DOWNSTREAM)) {
      return -EINVAL;
   }

   if (!virtusb_port_speed_is_valid(speed)) {
      return -EINVAL;
   }

   memset(port, 0, sizeof(*port));

   port->role = role;
   port->owner = owner;
   port->speed = speed;

   return 0;
}

int virtusb_port_attach(struct virtusb_port *downstream,
                        struct virtusb_port *upstream)
{
   if ((downstream == NULL) || (upstream == NULL)) {
      return -EINVAL;
   }

   if ((downstream->role != VIRTUSB_PORT_ROLE_DOWNSTREAM) ||
       (upstream->role != VIRTUSB_PORT_ROLE_UPSTREAM)) {
      return -EINVAL;
   }

   if ((downstream->peer != NULL) || (upstream->peer != NULL)) {
      return -EBUSY;
   }

   downstream->peer = upstream;
   upstream->peer = downstream;

   return 0;
}

void virtusb_port_detach(struct virtusb_port *port)
{
   struct virtusb_port *peer;

   if ((port == NULL) || (port->peer == NULL)) {
      return;
   }

   peer = port->peer;

   port->peer = NULL;

   if (peer->peer == port) {
      peer->peer = NULL;
   }
}

bool virtusb_port_is_attached(const struct virtusb_port *port)
{
   return (port != NULL) && (port->peer != NULL);
}

bool virtusb_port_has_vbus(const struct virtusb_port *port)
{
   const struct virtusb_port *peer;

   if ((port == NULL) || (port->role != VIRTUSB_PORT_ROLE_UPSTREAM)) {
      return false;
   }

   peer = port->peer;
   if ((peer == NULL) || (peer->role != VIRTUSB_PORT_ROLE_DOWNSTREAM)) {
      return false;
   }

   return peer->state.downstream.powered;
}

int virtusb_port_set_connection_signaling(struct virtusb_port *port,
                                          bool enabled)
{
   if ((port == NULL) || (port->role != VIRTUSB_PORT_ROLE_UPSTREAM)) {
      return -EINVAL;
   }

   port->state.upstream.connection_signaling = enabled;

   return 0;
}

int virtusb_port_set_powered(struct virtusb_port *port, bool powered)
{
   if ((port == NULL) || (port->role != VIRTUSB_PORT_ROLE_DOWNSTREAM)) {
      return -EINVAL;
   }

   port->state.downstream.powered = powered;

   return 0;
}

int virtusb_port_set_over_current(struct virtusb_port *port,
                                  bool over_current)
{
   if ((port == NULL) || (port->role != VIRTUSB_PORT_ROLE_DOWNSTREAM)) {
      return -EINVAL;
   }

   port->state.downstream.over_current = over_current;

   return 0;
}
