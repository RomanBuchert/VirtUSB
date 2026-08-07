// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/build_bug.h>
#include <linux/types.h>

/**
 * DOC: VirtUSB hub model
 *
 * This header defines the common functional hub and port state model used by
 * both VirtUsbRHub and VirtUsbHub.
 *
 * Boolean hub and port states are represented as 32-bit bitmaps:
 *
 * - bit 0 represents the hub itself,
 * - bits 1 through 31 represent downstream ports 1 through 31.
 *
 * Port numbers therefore map directly to their corresponding bitmap bit.
 *
 * Multi-bit properties use the same logical numbering. Their values are packed
 * into 32-bit words. The number of bits reserved per position must be a power
 * of two and must divide the word size without remainder.
 *
 * The model is independent of the Linux HCD interface and of the USB hub-class
 * protocol representation. Translation to and from Linux- or USB-specific
 * representations is performed by the corresponding adapter layer.
 */

#define VIRTUSB_MAX_HUB_PORTS      31U
#define VIRTUSB_HUB_POSITION_COUNT (VIRTUSB_MAX_HUB_PORTS + 1U)
#define VIRTUSB_STATE_WORD_BITS    32U

#define VIRTUSB_PACKED_BITS_VALID(_bits)                                      \
   (((_bits) != 0U) && (((_bits) & ((_bits) - 1U)) == 0U) &&                  \
    ((VIRTUSB_STATE_WORD_BITS % (_bits)) == 0U))

#define VIRTUSB_PACKED_VALUES_PER_WORD(_bits) \
   (VIRTUSB_STATE_WORD_BITS / (_bits))

#define VIRTUSB_PACKED_WORD_COUNT(_bits)                                      \
   ((VIRTUSB_HUB_POSITION_COUNT + VIRTUSB_PACKED_VALUES_PER_WORD(_bits) - 1U) \
    / VIRTUSB_PACKED_VALUES_PER_WORD(_bits))

#define VIRTUSB_PACKED_VALUE_MASK(_bits) ((1U << (_bits)) - 1U)

#define VIRTUSB_PORT_SPEED_BITS 2U

static_assert(VIRTUSB_PACKED_BITS_VALID(VIRTUSB_PORT_SPEED_BITS));

/**
 * enum virtusb_port_speed - USB speed associated with a downstream port
 * @VIRTUSB_PORT_SPEED_NONE: No USB speed is currently assigned.
 * @VIRTUSB_PORT_SPEED_LOW: Low-Speed USB connection.
 * @VIRTUSB_PORT_SPEED_FULL: Full-Speed USB connection.
 * @VIRTUSB_PORT_SPEED_HIGH: High-Speed USB connection.
 *
 * The values are represented internally using VIRTUSB_PORT_SPEED_BITS bits per
 * hub or port position.
 *
 * All possible values of the current two-bit representation correspond to
 * valid enum values.
 */
enum virtusb_port_speed {
   VIRTUSB_PORT_SPEED_NONE = 0,
   VIRTUSB_PORT_SPEED_LOW,
   VIRTUSB_PORT_SPEED_FULL,
   VIRTUSB_PORT_SPEED_HIGH,
};

/**
 * struct virtusb_hub_port_state - Hub and downstream-port states
 * @powered: Ports or hub currently powered.
 * @connected: Ports with a connected USB device.
 * @enabled: Ports currently enabled.
 * @suspended: Ports currently suspended.
 * @reset: Ports currently in reset state.
 * @over_current: Ports or hub currently reporting an over-current condition.
 * @speed: Packed USB speed associated with each hub or port position.
 *
 * Boolean members are bitmaps. Bit 0 is reserved for the hub itself and bits 1
 * through 31 correspond directly to downstream port numbers.
 *
 * Not every boolean state necessarily has meaningful hub-level semantics.
 * Bit 0 is nevertheless reserved consistently so that all state bitmaps use
 * the same addressing model.
 *
 * @speed uses VIRTUSB_PORT_SPEED_BITS bits per position. Positions are packed
 * consecutively into 32-bit words. Position 0 is reserved for the hub itself,
 * even though USB speed is normally meaningful only for downstream ports.
 *
 * The speed representation must be accessed through
 * virtusb_hub_get_port_speed() and virtusb_hub_set_port_speed().
 */
struct virtusb_hub_port_state {
   u32 powered;
   u32 connected;
   u32 enabled;
   u32 suspended;
   u32 reset;
   u32 over_current;

   u32 speed[VIRTUSB_PACKED_WORD_COUNT(VIRTUSB_PORT_SPEED_BITS)];
};

/**
 * struct virtusb_hub_port_change - Boolean hub and downstream-port change states
 * @connected: Connection state changed.
 * @enabled: Enable state changed.
 * @suspended: Suspend state changed.
 * @reset: Reset state changed.
 * @over_current: Over-current state changed.
 *
 * Each member is a bitmap using the same bit assignment as
 * struct virtusb_hub_port_state.
 *
 * A set bit indicates that the corresponding state has changed and that the
 * change has not yet been acknowledged by the consumer of the hub state.
 *
 * USB speed has no separate change bitmap. A speed change is associated with
 * the corresponding connection state.
 */
struct virtusb_hub_port_change {
   u32 connected;
   u32 enabled;
   u32 suspended;
   u32 reset;
   u32 over_current;
};

/**
 * struct virtusb_hub - Common functional state of a virtual USB hub
 * @port_count: Number of downstream ports provided by the hub.
 * @state: Current hub and port states.
 * @change: Pending hub and port state changes.
 *
 * This structure represents the common functional hub model shared by
 * VirtUsbRHub and VirtUsbHub.
 *
 * @port_count must be in the range 1 through VIRTUSB_MAX_HUB_PORTS.
 *
 * The structure intentionally contains no Linux HCD state, USB protocol
 * representation, synchronization primitives, or device ownership information.
 * Such information is added by the respective implementation layer when
 * required.
 */
struct virtusb_hub {
   u8 port_count;

   struct virtusb_hub_port_state state;
   struct virtusb_hub_port_change change;
};

/**
 * virtusb_hub_init() - Initialize a virtual USB hub state
 * @hub: Hub to initialize.
 * @port_count: Number of downstream ports.
 *
 * Initializes @hub with @port_count downstream ports and resets all hub and
 * port states.
 *
 * Return: 0 on success or a negative error code if the parameters are invalid.
 */
int virtusb_hub_init(struct virtusb_hub *hub, unsigned int port_count);

/**
 * virtusb_hub_reset() - Reset the complete functional hub state
 * @hub: Hub to reset.
 *
 * Resets all current and pending change states while preserving the configured
 * number of downstream ports.
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
 * virtusb_hub_get_port_speed() - Get the USB speed of a downstream port
 * @hub: Hub containing the downstream port.
 * @port_number: One-based downstream port number.
 *
 * Return: USB speed assigned to the port, or VIRTUSB_PORT_SPEED_NONE if the
 * port number is invalid.
 */
enum virtusb_port_speed
virtusb_hub_get_port_speed(const struct virtusb_hub *hub,
                           unsigned int port_number);

/**
 * virtusb_hub_set_port_speed() - Set the USB speed of a downstream port
 * @hub: Hub containing the downstream port.
 * @port_number: One-based downstream port number.
 * @speed: USB speed to assign.
 *
 * Return: 0 on success or a negative error code if the parameters are invalid.
 */
int virtusb_hub_set_port_speed(struct virtusb_hub *hub,
                               unsigned int port_number,
                               enum virtusb_port_speed speed);
