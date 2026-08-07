// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/build_bug.h>
#include <linux/types.h>

#include <virtusb_uapi.h>

/**
 * DOC: VirtUSB hub model
 *
 * This header defines the common functional hub and port state model used by
 * both VirtUsbRHub and VirtUsbHub.
 *
 * VirtUSB distinguishes between virtual hardware state and USB-visible state.
 *
 * The virtual hardware state represents externally controlled or simulated
 * conditions such as power availability, device attachment, over-current
 * conditions, and connection speed.
 *
 * The USB-visible state represents the state exposed through USB hub semantics
 * to the host. A hardware-state transition may cause a USB-visible state
 * transition, but the two representations are intentionally independent.
 *
 * Hub and port numbering, packed-state representation, and USB speed values
 * are defined by the shared VirtUSB user-space API in <virtusb_uapi.h>.
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
 * struct virtusb_hub_port_hw_state - Virtual hardware state of a hub and ports
 * @powered: Hub or ports with simulated hardware power available.
 * @connected: Ports with a simulated physically connected USB device.
 * @over_current: Hub or ports with a simulated over-current condition.
 * @speed: Packed simulated connection speed for each hub or port.
 *
 * This structure represents externally controlled virtual hardware conditions.
 * It does not directly represent USB hub status as visible to the host.
 *
 * Boolean members are bitmaps. Bit 0 represents the hub itself and bits 1
 * through 31 correspond directly to downstream port numbers.
 *
 * @speed uses VIRTUSB_PORT_SPEED_BITS bits per hub or port. Values are packed
 * consecutively into words according to the representation defined by the
 * VirtUSB UAPI.
 *
 * Position 0 is reserved for the hub itself even though connection speed is
 * normally meaningful only for downstream ports.
 */
struct virtusb_hub_port_hw_state {
   u32 powered;
   u32 connected;
   u32 over_current;

   u32 speed[VIRTUSB_PACKED_WORD_COUNT(VIRTUSB_PORT_SPEED_BITS)];
};

/**
 * struct virtusb_hub_port_usb_state - USB-visible state of a hub and ports
 * @powered: Current USB-visible logical power state.
 * @connected: Current USB-visible connection state.
 * @enabled: Ports currently enabled by USB operation.
 * @suspended: Ports currently suspended or resuming.
 * @reset: Ports currently receiving reset signaling.
 * @over_current: Current USB-visible over-current state.
 * @speed: Packed USB-visible connection speed for each hub or port.
 *
 * This structure represents the functional USB state exposed by the virtual
 * hub to the host.
 *
 * Some members may be derived from virtual hardware state, while others are
 * controlled by USB protocol or HCD operation.
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
 * VirtUSB UAPI.
 *
 * The packed speed representation must be accessed through
 * virtusb_hub_get_port_speed() and virtusb_hub_set_port_speed() where
 * applicable.
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
 * @connected: USB-visible connection status changed due to attach or detach.
 * @enabled: Port disable caused by a port error or equivalent USB condition.
 * @suspended: Resume processing completed.
 * @reset: Reset processing completed.
 * @over_current: USB-visible over-current state changed.
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
 * Virtual hardware state and USB-visible state are intentionally stored
 * separately. Changes to @hw may cause transitions in @usb and @usb_change,
 * but must not implicitly modify them without the corresponding hub-model
 * processing.
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
 * Initializes @hub with @port_count downstream ports and resets all virtual
 * hardware, USB-visible, and pending USB change state.
 *
 * Return: 0 on success or a negative error code if the parameters are invalid.
 */
int virtusb_hub_init(struct virtusb_hub *hub, unsigned int port_count);

/**
 * virtusb_hub_reset() - Reset the complete functional hub state
 * @hub: Hub to reset.
 *
 * Resets all virtual hardware, USB-visible, and pending USB change state while
 * preserving the configured number of downstream ports.
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

/**
 * virtusb_hub_set_port_speed() - Set the USB-visible speed of a downstream port
 * @hub: Hub containing the downstream port.
 * @port_number: One-based downstream port number.
 * @speed: USB-visible speed to assign.
 *
 * This function modifies the USB-visible speed representation. It does not
 * modify the simulated hardware connection speed.
 *
 * Return: 0 on success or a negative error code if the parameters are invalid.
 */
int virtusb_hub_set_port_speed(struct virtusb_hub *hub,
                               unsigned int port_number,
                               enum virtusb_port_speed speed);
