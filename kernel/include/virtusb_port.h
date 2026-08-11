// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/types.h>

/**
 * DOC: VirtUSB port model
 *
 * VirtUsbPort is the canonical internal representation of one virtual USB
 * port. A port is an integral subobject of its owning Core Component and has no
 * independent object identity or lifetime.
 *
 * The same type is used for upstream and downstream ports. Common fields
 * describe topology and local hardware capabilities. Role-specific hardware
 * state is stored in the corresponding union member.
 *
 * USB-defined protocol state such as PORT_CONNECTION, PORT_ENABLE,
 * PORT_SUSPEND, PORT_RESET, and the current operating speed is intentionally
 * not stored here.
 */

#define VIRTUSB_PORT_SPEED_CAP_LOW  (1U << 0)
#define VIRTUSB_PORT_SPEED_CAP_FULL (1U << 1)
#define VIRTUSB_PORT_SPEED_CAP_HIGH (1U << 2)

#define VIRTUSB_PORT_SPEED_CAP_ALL                                           \
   (VIRTUSB_PORT_SPEED_CAP_LOW | VIRTUSB_PORT_SPEED_CAP_FULL |               \
    VIRTUSB_PORT_SPEED_CAP_HIGH)

/**
 * enum virtusb_port_role - Direction of a VirtUSB port
 * @VIRTUSB_PORT_ROLE_INVALID: Invalid or uninitialized port.
 * @VIRTUSB_PORT_ROLE_UPSTREAM: Upstream-facing port owned by a device or hub.
 * @VIRTUSB_PORT_ROLE_DOWNSTREAM: Downstream-facing port owned by a hub.
 */
enum virtusb_port_role {
   VIRTUSB_PORT_ROLE_INVALID = 0,
   VIRTUSB_PORT_ROLE_UPSTREAM,
   VIRTUSB_PORT_ROLE_DOWNSTREAM,
};

/**
 * struct virtusb_port_upstream_state - Upstream-specific hardware state
 * @connection_signaling: Device currently signals USB presence.
 *
 * This state represents the USB-relevant result of device-side connect or
 * disconnect control. It does not model a physical pull-up resistor or any
 * internal device power, firmware, or controller state.
 */
struct virtusb_port_upstream_state {
   bool connection_signaling;
};

/**
 * struct virtusb_port_downstream_state - Downstream-specific hardware state
 * @powered: VBUS is currently present at this downstream port.
 * @over_current: A per-port over-current condition is currently present.
 */
struct virtusb_port_downstream_state {
   bool powered;
   bool over_current;
};

/**
 * struct virtusb_port - Canonical virtual USB port representation
 * @role: Upstream or downstream role of this port.
 * @owner: Core-component subobject that owns this port.
 * @speed: Bit mask of VIRTUSB_PORT_SPEED_CAP_* hardware capabilities.
 * @peer: Attached peer port, or NULL when detached.
 * @state: Role-specific local hardware state.
 *
 * Attachment is represented exclusively through reciprocal @peer references.
 * No separate attached or associated state is maintained.
 */
struct virtusb_port {
   enum virtusb_port_role role;
   void *owner;
   u8 speed;
   struct virtusb_port *peer;

   union {
      struct virtusb_port_upstream_state upstream;
      struct virtusb_port_downstream_state downstream;
   } state;
};

/**
 * virtusb_port_init() - Initialize a VirtUSB port
 * @port: Port to initialize.
 * @role: Role of the port.
 * @speed: Supported-speed capability mask.
 * @owner: Owning Core Component or common hub subobject.
 *
 * @speed must contain at least one VIRTUSB_PORT_SPEED_CAP_* bit and no unknown
 * bits.
 *
 * Return: 0 on success or a negative error code on invalid parameters.
 */
int virtusb_port_init(struct virtusb_port *port,
                      enum virtusb_port_role role,
                      u8 speed,
                      void *owner);

/**
 * virtusb_port_attach() - Attach one downstream and one upstream port
 * @downstream: Detached downstream port.
 * @upstream: Detached upstream port.
 *
 * Establishes the reciprocal peer relationship representing a virtual USB
 * cable attachment.
 *
 * Callers must serialize topology changes.
 *
 * Return: 0 on success or a negative error code if the parameters, roles, or
 * current attachment state are invalid.
 */
int virtusb_port_attach(struct virtusb_port *downstream,
                        struct virtusb_port *upstream);

/**
 * virtusb_port_detach() - Detach a port from its peer
 * @port: Port to detach.
 *
 * Clears both sides of the reciprocal peer relationship. Detaching an already
 * detached port has no effect.
 *
 * Callers must serialize topology changes.
 */
void virtusb_port_detach(struct virtusb_port *port);

/**
 * virtusb_port_is_attached() - Test whether a port has an attached peer
 * @port: Port to test.
 *
 * Return: true if @port has a peer, otherwise false.
 */
bool virtusb_port_is_attached(const struct virtusb_port *port);

/**
 * virtusb_port_has_vbus() - Test whether VBUS is present at an upstream port
 * @port: Upstream port to query.
 *
 * VBUS is derived from the attached downstream peer and is not stored as
 * duplicated upstream state.
 *
 * Return: true if @port is upstream, attached to a downstream port, and that
 * downstream port is powered; otherwise false.
 */
bool virtusb_port_has_vbus(const struct virtusb_port *port);

/**
 * virtusb_port_set_connection_signaling() - Set upstream connection signaling
 * @port: Upstream port to modify.
 * @enabled: New connection-signaling state.
 *
 * Return: 0 on success or a negative error code for a non-upstream port.
 */
int virtusb_port_set_connection_signaling(struct virtusb_port *port,
                                          bool enabled);

/**
 * virtusb_port_set_powered() - Set downstream VBUS state
 * @port: Downstream port to modify.
 * @powered: New VBUS state.
 *
 * Return: 0 on success or a negative error code for a non-downstream port.
 */
int virtusb_port_set_powered(struct virtusb_port *port, bool powered);

/**
 * virtusb_port_set_over_current() - Set downstream over-current state
 * @port: Downstream port to modify.
 * @over_current: New per-port over-current state.
 *
 * Return: 0 on success or a negative error code for a non-downstream port.
 */
int virtusb_port_set_over_current(struct virtusb_port *port,
                                  bool over_current);
