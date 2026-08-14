// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/build_bug.h>
#include <linux/types.h>

#include <virtusb_uapi.h>

#include "virtusb_port.h"

/**
 * DOC: VirtUSB hub model
 *
 * This header defines the common functional hub model used by VirtUsbRHub and
 * VirtUsbHub.
 *
 * Downstream hardware state is stored canonically in individual VirtUsbPort
 * instances. USB-defined hub-port protocol state is kept separately.
 * Aggregate hardware bitmaps are not maintained as a second source of truth.
 */

static_assert(VIRTUSB_PACKED_BITS_VALID(VIRTUSB_PORT_SPEED_BITS));

struct virtusb_hub;

/**
 * typedef virtusb_hub_status_changed_t - Hub status-change notification
 * @hub: Hub with newly pending USB-defined change information.
 * @context: Opaque notification-consumer context.
 *
 * The callback reports only that new USB-defined hub or port change
 * information became pending. It does not define how that information is
 * transported to the host.
 */
typedef void (*virtusb_hub_status_changed_t)(struct virtusb_hub *hub,
                                             void *context);

/**
 * enum virtusb_hub_power_switching_mode - Downstream VBUS switching capability
 * @VIRTUSB_HUB_POWER_SWITCHING_GANGED: All downstream ports switch together.
 * @VIRTUSB_HUB_POWER_SWITCHING_INDIVIDUAL: Ports switch independently.
 */
enum virtusb_hub_power_switching_mode {
   VIRTUSB_HUB_POWER_SWITCHING_GANGED = 0,
   VIRTUSB_HUB_POWER_SWITCHING_INDIVIDUAL,
};

/**
 * enum virtusb_hub_over_current_mode - Hub over-current reporting capability
 * @VIRTUSB_HUB_OVER_CURRENT_GLOBAL: One hub-wide over-current condition.
 * @VIRTUSB_HUB_OVER_CURRENT_PER_PORT: One condition per downstream port.
 * @VIRTUSB_HUB_OVER_CURRENT_NONE: No over-current detection.
 */
enum virtusb_hub_over_current_mode {
   VIRTUSB_HUB_OVER_CURRENT_GLOBAL = 0,
   VIRTUSB_HUB_OVER_CURRENT_PER_PORT,
   VIRTUSB_HUB_OVER_CURRENT_NONE,
};

/**
 * struct virtusb_hub_port_usb_state - USB protocol state of downstream ports
 * @enabled: USB-defined PORT_ENABLE state.
 * @suspended: USB-defined PORT_SUSPEND state.
 * @reset: USB-defined PORT_RESET state.
 * @speed: Packed current USB operating speed for each hub or port.
 *
 * Hardware facts such as Attachment, VBUS, connection signaling, and physical
 * over-current conditions are not duplicated in this structure.
 */
struct virtusb_hub_port_usb_state {
   u32 enabled;
   u32 suspended;
   u32 reset;

   u32 speed[VIRTUSB_PACKED_WORD_COUNT(VIRTUSB_PORT_SPEED_BITS)];
};

/**
 * struct virtusb_hub_port_usb_change - USB hub and port change information
 * @connected: USB-defined C_PORT_CONNECTION condition.
 * @enabled: USB-defined C_PORT_ENABLE condition.
 * @suspended: USB-defined C_PORT_SUSPEND condition.
 * @reset: USB-defined C_PORT_RESET condition.
 * @over_current: USB-defined C_PORT_OVER_CURRENT condition.
 *
 * Pending change information remains set until acknowledged according to USB
 * hub semantics.
 */
struct virtusb_hub_port_usb_change {
   u32 connected;
   u32 enabled;
   u32 suspended;
   u32 reset;
   u32 over_current;
};

/**
 * struct virtusb_hub - Common functional state of a virtual USB hub
 * @port_count: Number of downstream ports.
 * @power_switching_mode: Hardware power-switching capability.
 * @over_current_mode: Hardware over-current capability.
 * @over_current: Current global over-current condition.
 * @ports: Canonical downstream VirtUsbPort instances.
 * @usb: USB-defined protocol state.
 * @usb_change: Pending USB-defined change information.
 * @status_changed: Optional transport-independent change notification.
 * @status_changed_context: Opaque context passed to @status_changed.
 *
 * Root hubs currently initialize with individual power switching and per-port
 * over-current detection, matching the existing VirtUSB behavior. These
 * capabilities can be changed through the hub-model setters before the hub is
 * exposed.
 */
struct virtusb_hub {
   u8 port_count;

   enum virtusb_hub_power_switching_mode power_switching_mode;
   enum virtusb_hub_over_current_mode over_current_mode;

   bool over_current;

   struct virtusb_port ports[VIRTUSB_MAX_HUB_PORTS];

   struct virtusb_hub_port_usb_state usb;
   struct virtusb_hub_port_usb_change usb_change;

   virtusb_hub_status_changed_t status_changed;
   void *status_changed_context;
};

int virtusb_hub_init(struct virtusb_hub *hub, unsigned int port_count);

void virtusb_hub_reset(struct virtusb_hub *hub);

void virtusb_hub_set_status_changed_callback(
   struct virtusb_hub *hub,
   virtusb_hub_status_changed_t callback,
   void *context);

bool virtusb_hub_port_is_valid(const struct virtusb_hub *hub,
                               unsigned int port_number);

struct virtusb_port *virtusb_hub_get_port(struct virtusb_hub *hub,
                                          unsigned int port_number);

const struct virtusb_port *
virtusb_hub_get_port_const(const struct virtusb_hub *hub,
                           unsigned int port_number);

int virtusb_hub_find_port_number(const struct virtusb_hub *hub,
                                 const struct virtusb_port *port,
                                 unsigned int *port_number);

void virtusb_hub_mark_port_connection_change(struct virtusb_hub *hub,
                                              unsigned int port_number);


/**
 * virtusb_hub_reset_port() - Perform and complete a USB port reset
 * @hub: Hub containing the downstream port.
 * @port_number: One-based downstream port number.
 *
 * VirtUSB currently models reset completion synchronously. A connected port
 * is disabled, reset, assigned the effective operating speed derived from both
 * attached port capabilities, re-enabled, and marked with C_PORT_RESET.
 *
 * The reset-completion timing required by a physical USB bus is not simulated
 * at this implementation stage.
 *
 * Return: 0 on success or a negative error code if the port is invalid,
 * disconnected, or has no compatible USB speed.
 */
int virtusb_hub_reset_port(struct virtusb_hub *hub,
                           unsigned int port_number);

int virtusb_hub_set_power_switching_mode(
   struct virtusb_hub *hub,
   enum virtusb_hub_power_switching_mode mode);

int virtusb_hub_set_over_current_mode(
   struct virtusb_hub *hub,
   enum virtusb_hub_over_current_mode mode);

int virtusb_hub_set_port_power(struct virtusb_hub *hub,
                               unsigned int port_number,
                               bool powered);

int virtusb_hub_set_all_ports_power(struct virtusb_hub *hub, bool powered);

bool virtusb_hub_port_is_powered(const struct virtusb_hub *hub,
                                 unsigned int port_number);

int virtusb_hub_set_port_over_current(struct virtusb_hub *hub,
                                      unsigned int port_number,
                                      bool over_current);

bool virtusb_hub_port_is_over_current(const struct virtusb_hub *hub,
                                      unsigned int port_number);

void virtusb_hub_set_over_current(struct virtusb_hub *hub, bool over_current);

bool virtusb_hub_is_over_current(const struct virtusb_hub *hub);

bool virtusb_hub_port_is_connected(const struct virtusb_hub *hub,
                                   unsigned int port_number);

enum virtusb_port_speed
virtusb_hub_get_port_speed(const struct virtusb_hub *hub,
                           unsigned int port_number);
