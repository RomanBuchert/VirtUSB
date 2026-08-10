// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/build_bug.h>
#include <linux/types.h>

#include <virtusb_uapi.h>

/**
 * DOC: VirtUSB hub model
 *
 * This header defines the common functional hub and downstream-port state model
 * used by both VirtUsbRHub and VirtUsbHub.
 *
 * VirtUSB keeps simulated hardware conditions separate from USB-visible hub
 * state. USB-visible state follows USB hub semantics wherever practical.
 *
 * The virtual hardware state contains only conditions that are external to the
 * USB hub state machine, such as a simulated over-current condition. Topology
 * state such as device attachment is not stored here and will be represented by
 * the VirtUSB device/topology model.
 *
 * The USB-visible state corresponds to USB-defined hub and port status such as
 * PORT_POWER, PORT_CONNECTION, PORT_ENABLE, PORT_SUSPEND, PORT_RESET, and
 * PORT_OVER_CURRENT. These fields are not generic writable hardware flags.
 *
 * Hub and port numbering, packed-state representation, and USB speed values are
 * defined by the shared VirtUSB user-space API in <virtusb_uapi.h>.
 *
 * Boolean hub and port states are represented as 32-bit bitmaps:
 *
 * - bit 0 represents the hub itself,
 * - bits 1 through 31 represent downstream ports 1 through 31.
 *
 * Port numbers therefore map directly to their corresponding bitmap bit.
 *
 * Multi-bit properties use the same logical numbering. Their values are packed
 * into 32-bit words according to the representation defined by the VirtUSB
 * UAPI.
 *
 * The functional hub model remains independent of the Linux HCD interface and
 * of the USB hub-class protocol representation. Translation to and from
 * Linux- or USB-specific representations is performed by the corresponding
 * adapter layer.
 */

static_assert(VIRTUSB_PACKED_BITS_VALID(VIRTUSB_PORT_SPEED_BITS));

/**
 * struct virtusb_hub_port_hw_state - Simulated hardware conditions of a hub and ports
 * @over_current: Hub or ports with a simulated over-current condition.
 *
 * This structure contains only simulated hardware conditions that are external
 * to the USB-visible hub-port state machine.
 *
 * Device attachment is topology state and is intentionally not represented
 * here. USB logical port power and USB connection state are represented by
 * struct virtusb_hub_port_usb_state.
 *
 * Boolean members are bitmaps. Bit 0 represents the hub itself and bits 1
 * through 31 correspond directly to downstream port numbers.
 */
struct virtusb_hub_port_hw_state {
   u32 over_current;
};

/**
 * struct virtusb_hub_port_usb_state - USB-visible state of a hub and ports
 * @powered: USB-defined logical PORT_POWER state.
 * @connected: USB-defined PORT_CONNECTION state.
 * @enabled: USB-defined PORT_ENABLE state.
 * @suspended: USB-defined PORT_SUSPEND state.
 * @reset: USB-defined PORT_RESET state.
 * @over_current: USB-defined PORT_OVER_CURRENT state.
 * @speed: Packed USB-visible connection speed for each hub or port.
 *
 * This structure represents the functional USB state exposed by the virtual
 * hub to the host.
 *
 * Some members are derived from topology or simulated hardware conditions,
 * while others are controlled by USB hub operation. In particular,
 * @connected is derived state and must not be treated as an independently
 * writable attachment flag. Likewise, @powered represents USB logical port
 * power and is not Device Power.
 *
 * Boolean members are bitmaps. Bit 0 represents the hub itself and bits 1
 * through 31 correspond directly to downstream port numbers.
 *
 * Not every state necessarily has meaningful hub-level semantics. Bit 0 is
 * nevertheless reserved consistently so that all state representations use
 * the same addressing model.
 *
 * @speed uses VIRTUSB_PORT_SPEED_BITS bits per hub or port. Values are packed
 * consecutively into words according to the representation defined by the
 * VirtUSB UAPI. USB-visible speed is established by hub/device processing, not
 * by arbitrary external writes.
 */
struct virtusb_hub_port_usb_state {
   u32 powered;
   u32 connected;
   u32 enabled;
   u32 suspended;
   u32 reset;
   u32 over_current;

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
 * Each member is a bitmap using the common VirtUSB hub and port numbering
 * model.
 *
 * These fields model USB hub change semantics rather than generic edge
 * detection. A set bit therefore indicates the USB-defined change condition
 * associated with that state and not necessarily every transition of the
 * corresponding USB-visible state.
 *
 * Pending change information remains set until acknowledged by the appropriate
 * consumer or cleared as required by USB hub semantics.
 *
 * USB speed has no independent change bitmap. A newly detected connection
 * speed is associated with connection processing.
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
 * @port_count: Number of downstream ports provided by the hub.
 * @hw: Current virtual hardware state.
 * @usb: Current USB-visible state.
 * @usb_change: Pending USB hub and port change information.
 *
 * This structure represents the common functional hub model shared by
 * VirtUsbRHub and VirtUsbHub.
 *
 * @port_count must be in the range 1 through VIRTUSB_MAX_HUB_PORTS.
 *
 * Simulated hardware conditions and USB-visible state are intentionally stored
 * separately. Changes to @hw may cause transitions in @usb and @usb_change,
 * but only through the corresponding hub-model processing. Topology and device
 * ownership are intentionally not stored in this structure.
 *
 * The structure intentionally contains no Linux HCD state, USB protocol
 * representation, synchronization primitives, or device ownership information.
 * Such information is added by the respective implementation layer when
 * required.
 */
struct virtusb_hub {
   u8 port_count;

   struct virtusb_hub_port_hw_state hw;
   struct virtusb_hub_port_usb_state usb;
   struct virtusb_hub_port_usb_change usb_change;
};

/**
 * virtusb_hub_init() - Initialize a virtual USB hub state
 * @hub: Hub to initialize.
 * @port_count: Number of downstream ports.
 *
 * Initializes @hub with @port_count downstream ports and resets all simulated
 * hardware conditions, USB-visible state, and pending USB change state.
 *
 * Return: 0 on success or a negative error code if the parameters are invalid.
 */
int virtusb_hub_init(struct virtusb_hub *hub, unsigned int port_count);

/**
 * virtusb_hub_reset() - Reset the complete functional hub state
 * @hub: Hub to reset.
 *
 * Resets all simulated hardware conditions, USB-visible state, and pending USB
 * change state while preserving the configured number of downstream ports.
 */
void virtusb_hub_reset(struct virtusb_hub *hub);

/**
 * virtusb_hub_port_is_valid() - Check whether a downstream port number is valid
 * @hub: Hub containing the downstream port.
 * @port_number: One-based downstream port number.
 *
 * Return: true if @port_number refers to an existing downstream port, otherwise
 * false.
 */
bool virtusb_hub_port_is_valid(const struct virtusb_hub *hub,
                               unsigned int port_number);

/**
 * virtusb_hub_get_port_speed() - Get the USB-visible speed of a downstream port
 * @hub: Hub containing the downstream port.
 * @port_number: One-based downstream port number.
 *
 * Return: USB-visible speed assigned to the port, or
 * VIRTUSB_PORT_SPEED_NONE if the port number is invalid.
 */
enum virtusb_port_speed
virtusb_hub_get_port_speed(const struct virtusb_hub *hub,
                           unsigned int port_number);
