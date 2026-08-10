// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/types.h>

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

/**
 * DOC: VirtUSB user-space API
 *
 * This header defines data representations, constants, and ioctl commands
 * shared between the VirtUSB kernel module and Linux user-space components.
 *
 * The VirtUSB UAPI is experimental while its version is 0.0. Structures,
 * constants, command definitions, and semantics may change without
 * compatibility guarantees.
 *
 * Hub and port numbering follows a common model:
 *
 * - hub ID 0 represents the root hub of the VirtUsbHcd associated with the
 *   opened /dev/virtusbX endpoint,
 * - hub IDs greater than 0 identify ordinary VirtUsbHub instances within that
 *   HCD's topology,
 * - port 0 represents the addressed hub itself,
 * - ports 1 through 31 represent downstream ports 1 through 31.
 *
 * Boolean hub and port properties may be represented as 32-bit bitmaps in the
 * status-oriented Control-Plane view:
 *
 * - bit 0 represents the hub itself,
 * - bits 1 through 31 represent downstream ports 1 through 31.
 *
 * Multi-bit properties use the same numbering and are packed consecutively
 * into 32-bit words. The number of bits assigned to each hub or port must be a
 * power of two and must divide the packed word size without remainder.
 */

/**
 * DOC: VirtUSB UAPI version
 *
 * Version 0.0 denotes an experimental and unstable UAPI. The first defined
 * compatibility baseline is expected to be introduced with UAPI version 0.1.
 */
#define VIRTUSB_UAPI_VERSION_MAJOR 0U
#define VIRTUSB_UAPI_VERSION_MINOR 0U

/**
 * DOC: VirtUSB hub and port addressing
 *
 * Hub ID 0 is reserved for the VirtUsbRHub belonging to the VirtUsbHcd
 * represented by the opened /dev/virtusbX endpoint.
 *
 * Port 0 represents the addressed hub itself. Ports 1 through
 * VIRTUSB_MAX_HUB_PORTS represent downstream ports.
 */
#define VIRTUSB_ROOT_HUB_ID        0U
#define VIRTUSB_MAX_HUB_PORTS      31U
#define VIRTUSB_HUB_POSITION_COUNT (VIRTUSB_MAX_HUB_PORTS + 1U)

/**
 * DOC: VirtUSB packed property representation
 *
 * Multi-bit properties are packed into 32-bit words. Values are stored
 * consecutively according to hub and port numbering, starting with the hub
 * itself at port 0.
 */
#define VIRTUSB_PACKED_WORD_BITS 32U

#define VIRTUSB_PACKED_BITS_VALID(_bits)                                      \
   (((_bits) != 0U) && (((_bits) & ((_bits) - 1U)) == 0U) &&                  \
    ((VIRTUSB_PACKED_WORD_BITS % (_bits)) == 0U))

#define VIRTUSB_PACKED_VALUES_PER_WORD(_bits) \
   (VIRTUSB_PACKED_WORD_BITS / (_bits))

#define VIRTUSB_PACKED_WORD_COUNT(_bits)                                      \
   ((VIRTUSB_HUB_POSITION_COUNT + VIRTUSB_PACKED_VALUES_PER_WORD(_bits) - 1U) \
    / VIRTUSB_PACKED_VALUES_PER_WORD(_bits))

#define VIRTUSB_PACKED_VALUE_MASK(_bits) ((1U << (_bits)) - 1U)

/**
 * DOC: VirtUSB USB speed representation
 *
 * USB speed is represented using two bits per hub or port.
 */
#define VIRTUSB_PORT_SPEED_BITS 2U

/**
 * enum virtusb_port_speed - USB speed associated with a hub or port
 * @VIRTUSB_PORT_SPEED_NONE: No USB speed is currently assigned.
 * @VIRTUSB_PORT_SPEED_LOW: Low-Speed USB connection.
 * @VIRTUSB_PORT_SPEED_FULL: Full-Speed USB connection.
 * @VIRTUSB_PORT_SPEED_HIGH: High-Speed USB connection.
 *
 * All possible values of the current two-bit representation correspond to
 * valid enum values.
 *
 * ABI structures use __u32 rather than enum virtusb_port_speed directly so
 * that their binary field sizes remain explicitly defined.
 */
enum virtusb_port_speed {
   VIRTUSB_PORT_SPEED_NONE = 0,
   VIRTUSB_PORT_SPEED_LOW,
   VIRTUSB_PORT_SPEED_FULL,
   VIRTUSB_PORT_SPEED_HIGH,
};

/**
 * DOC: VirtUSB port status flags
 *
 * These flags are used by the port-oriented Control-Plane view to describe the
 * current USB-visible boolean state of one hub or downstream port.
 *
 * Bit assignments intentionally follow the corresponding USB 2.0 wPortStatus
 * layout where applicable.
 *
 * Bit 5 is intentionally unused and reserved for possible future PORT_L1
 * support.
 */
#define VIRTUSB_PORT_STATUS_CONNECTED    (1U << 0)
#define VIRTUSB_PORT_STATUS_ENABLED      (1U << 1)
#define VIRTUSB_PORT_STATUS_SUSPENDED    (1U << 2)
#define VIRTUSB_PORT_STATUS_OVER_CURRENT (1U << 3)
#define VIRTUSB_PORT_STATUS_RESET        (1U << 4)
#define VIRTUSB_PORT_STATUS_POWER        (1U << 8)

/**
 * DOC: VirtUSB port change flags
 *
 * These flags describe pending USB-defined status-change conditions associated
 * with one hub or downstream port.
 *
 * Bit assignments intentionally follow the corresponding USB 2.0 wPortChange
 * layout.
 *
 * Bit 5 is intentionally unused and reserved for possible future C_PORT_L1
 * support.
 */
#define VIRTUSB_PORT_CHANGE_CONNECTED    (1U << 0)
#define VIRTUSB_PORT_CHANGE_ENABLED      (1U << 1)
#define VIRTUSB_PORT_CHANGE_SUSPENDED    (1U << 2)
#define VIRTUSB_PORT_CHANGE_OVER_CURRENT (1U << 3)
#define VIRTUSB_PORT_CHANGE_RESET        (1U << 4)

/**
 * struct virtusb_port_state - Complete USB-visible state of one hub or port
 * @status: Current boolean state as VIRTUSB_PORT_STATUS_* flags.
 * @change: Pending USB change information as VIRTUSB_PORT_CHANGE_* flags.
 * @speed: USB speed represented by enum virtusb_port_speed values.
 */
struct virtusb_port_state {
   __u32 status;
   __u32 change;
   __u32 speed;
};

/**
 * struct virtusb_port_status - Port-oriented hub state query
 * @hub_id: Hub containing the addressed port.
 * @port: Hub or downstream-port number to query.
 * @port_count: Number of downstream ports provided by the addressed hub.
 * @state: State of the hub or port selected by @port.
 * @ports: Additional downstream-port states returned when @port is 0.
 *
 * VIRTUSB_ROOT_HUB_ID identifies the root hub.
 *
 * Port 0 addresses the hub itself. Ports 1 through @port_count address
 * downstream ports.
 *
 * For @port greater than 0, @state contains the complete USB-visible state of
 * the selected downstream port. The contents of @ports are undefined.
 *
 * For @port equal to 0, @state contains the state of the hub itself and
 * @ports[0] through @ports[@port_count - 1] contain the states of downstream
 * ports 1 through @port_count respectively.
 */
struct virtusb_port_status {
   __u32 hub_id;
   __u32 port;
   __u32 port_count;

   struct virtusb_port_state state;
   struct virtusb_port_state ports[VIRTUSB_MAX_HUB_PORTS];
};

/**
 * struct virtusb_status_bitmap - Status-oriented boolean property representation
 * @hub_id: Hub whose state bitmap is accessed.
 * @value: Boolean state bitmap.
 *
 * Bit 0 of @value represents the hub itself. Bits 1 through 31 represent
 * downstream ports 1 through 31.
 *
 * GET operations return the complete bitmap. Filtering for selected ports is
 * performed by user space.
 */
struct virtusb_status_bitmap {
   __u32 hub_id;
   __u32 value;
};

/**
 * struct virtusb_speed_map - Status-oriented USB speed representation
 * @hub_id: Hub whose USB speed map is accessed.
 * @value: Packed USB speed values.
 *
 * @value contains VIRTUSB_PORT_SPEED_BITS bits per hub or port using the common
 * VirtUSB numbering model.
 */
struct virtusb_speed_map {
   __u32 hub_id;
   __u32 value[VIRTUSB_PACKED_WORD_COUNT(VIRTUSB_PORT_SPEED_BITS)];
};

/**
 * DOC: VirtUSB ioctl interface
 *
 * The ioctl type value identifies VirtUSB Control-Plane commands.
 *
 * USB-visible hub-port state retains USB semantics. In particular,
 * PORT_CONNECTION and PORT_POWER are not exposed as arbitrary writable
 * Control-Plane properties. Writable commands operate only on VirtUSB-specific
 * management state or simulated hardware conditions.
 */
#define VIRTUSB_IOCTL_MAGIC 0xB9U

#define VIRTUSB_IOCTL_GET_PORT_STATUS \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x00, struct virtusb_port_status)

#define VIRTUSB_IOCTL_GET_POWER \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x01, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_GET_CONNECTED \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x02, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_GET_ENABLED \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x03, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_GET_SUSPENDED \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x04, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_GET_RESET \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x05, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_GET_OVER_CURRENT \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x06, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_GET_SPEED \
   _IOWR(VIRTUSB_IOCTL_MAGIC, 0x07, struct virtusb_speed_map)

#define VIRTUSB_IOCTL_SET_OVER_CURRENT \
   _IOW(VIRTUSB_IOCTL_MAGIC, 0x12, struct virtusb_status_bitmap)

#define VIRTUSB_IOCTL_SET_SPEED \
   _IOW(VIRTUSB_IOCTL_MAGIC, 0x13, struct virtusb_speed_map)
