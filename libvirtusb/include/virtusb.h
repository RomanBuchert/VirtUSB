// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: libvirtusb public API
 *
 * libvirtusb provides a small user-space C API for accessing VirtUSB host
 * controller instances exposed through /dev/virtusbX.
 *
 * The public library API is intentionally separate from the Linux kernel UAPI.
 * Applications using libvirtusb therefore do not need to include
 * <virtusb_uapi.h> or interact with ioctl() directly.
 */

#define VIRTUSB_MAX_PORTS 31U

/**
 * DOC: libvirtusb port status flags
 *
 * These flags describe the current USB-visible state returned by
 * virtusb_get_port_status().
 */
#define VIRTUSB_STATUS_CONNECTED    (1U << 0)
#define VIRTUSB_STATUS_ENABLED      (1U << 1)
#define VIRTUSB_STATUS_SUSPENDED    (1U << 2)
#define VIRTUSB_STATUS_OVER_CURRENT (1U << 3)
#define VIRTUSB_STATUS_RESET        (1U << 4)
#define VIRTUSB_STATUS_POWER        (1U << 8)

/**
 * DOC: libvirtusb port change flags
 *
 * These flags describe pending USB-defined change conditions returned by
 * virtusb_get_port_status().
 */
#define VIRTUSB_CHANGE_CONNECTED    (1U << 0)
#define VIRTUSB_CHANGE_ENABLED      (1U << 1)
#define VIRTUSB_CHANGE_SUSPENDED    (1U << 2)
#define VIRTUSB_CHANGE_OVER_CURRENT (1U << 3)
#define VIRTUSB_CHANGE_RESET        (1U << 4)

/**
 * enum virtusb_speed - USB speed exposed by libvirtusb
 * @VIRTUSB_SPEED_NONE: No USB speed is currently assigned.
 * @VIRTUSB_SPEED_LOW: Low-Speed USB connection.
 * @VIRTUSB_SPEED_FULL: Full-Speed USB connection.
 * @VIRTUSB_SPEED_HIGH: High-Speed USB connection.
 */
enum virtusb_speed {
   VIRTUSB_SPEED_NONE = 0,
   VIRTUSB_SPEED_LOW,
   VIRTUSB_SPEED_FULL,
   VIRTUSB_SPEED_HIGH,
};

/**
 * struct virtusb_state - State of one hub or downstream port
 * @status: Current state as VIRTUSB_STATUS_* flags.
 * @change: Pending change information as VIRTUSB_CHANGE_* flags.
 * @speed: Current USB speed.
 */
struct virtusb_state {
   uint32_t status;
   uint32_t change;
   enum virtusb_speed speed;
};

/**
 * struct virtusb_status - Result of a VirtUSB port-status query
 * @hub_id: Hub that was queried.
 * @port: Hub or downstream-port number that was queried.
 * @port_count: Number of downstream ports provided by the hub.
 * @state: State of the requested hub or downstream port.
 * @ports: States of all downstream ports when @port is zero.
 *
 * Port zero represents the hub itself. For @port greater than zero, @state
 * contains the requested downstream-port state and @ports is zero-filled.
 *
 * For @port equal to zero, @state contains the hub state and @ports[0] through
 * @ports[@port_count - 1] contain downstream ports 1 through @port_count.
 */
struct virtusb_status {
   uint32_t hub_id;
   uint32_t port;
   uint32_t port_count;

   struct virtusb_state state;
   struct virtusb_state ports[VIRTUSB_MAX_PORTS];
};

struct virtusb_handle;

typedef uint32_t virtusb_object_id_t;

#define VIRTUSB_INVALID_OBJECT_ID ((virtusb_object_id_t)0U)

#define VIRTUSB_DEVICE_SPEED_LOW  (1U << 0)
#define VIRTUSB_DEVICE_SPEED_FULL (1U << 1)
#define VIRTUSB_DEVICE_SPEED_HIGH (1U << 2)
#define VIRTUSB_DEVICE_SPEED_ALL \
   (VIRTUSB_DEVICE_SPEED_LOW | VIRTUSB_DEVICE_SPEED_FULL | VIRTUSB_DEVICE_SPEED_HIGH)

/**
 * virtusb_open() - Open one VirtUSB host-controller instance
 * @instance: Zero-based VirtUSB HCD instance number.
 * @handle: Receives the allocated library handle.
 *
 * Opens /dev/virtusbX where X equals @instance.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_open(unsigned int instance, struct virtusb_handle **handle);

/**
 * virtusb_close() - Close a VirtUSB library handle
 * @handle: Handle returned by virtusb_open().
 *
 * Passing NULL has no effect.
 */
void virtusb_close(struct virtusb_handle *handle);

/**
 * virtusb_get_device_path() - Get the device path of an open VirtUSB handle
 * @handle: Open VirtUSB handle.
 *
 * Return: Pointer to the handle-owned device-path string, or NULL if @handle is
 * NULL. The returned pointer remains valid until virtusb_close() is called.
 */
const char *virtusb_get_device_path(const struct virtusb_handle *handle);

/**
 * virtusb_get_port_status() - Query one hub or downstream port
 * @handle: Open VirtUSB handle.
 * @hub_id: Hub to query.
 * @port: Hub or downstream-port number. Zero addresses the hub itself.
 * @status: Receives the query result.
 *
 * Querying port zero additionally returns the state of all downstream ports in
 * @status->ports.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_get_port_status(struct virtusb_handle *handle,
                            uint32_t hub_id,
                            uint32_t port,
                            struct virtusb_status *status);

/**
 * virtusb_device_create() - Create a virtual USB device
 * @handle: Open VirtUSB Control-Plane handle.
 * @speed_caps: Supported-speed capability mask.
 * @object_id: Receives the global runtime object ID.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_device_create(struct virtusb_handle *handle,
                          uint32_t speed_caps,
                          virtusb_object_id_t *object_id);

/**
 * virtusb_device_destroy() - Destroy a virtual USB device
 * @handle: Open VirtUSB Control-Plane handle.
 * @object_id: Global runtime object ID.
 * @force: Request explicit forced destruction.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_device_destroy(struct virtusb_handle *handle,
                           virtusb_object_id_t object_id,
                           bool force);

/**
 * virtusb_device_attach() - Attach a device to a downstream port
 * @handle: Open VirtUSB Control-Plane handle selecting the HCD.
 * @object_id: Global runtime object ID.
 * @hub_id: Hub containing the target downstream port.
 * @port: One-based downstream port number.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_device_attach(struct virtusb_handle *handle,
                          virtusb_object_id_t object_id,
                          uint32_t hub_id,
                          uint32_t port);

/**
 * virtusb_device_detach() - Detach a virtual USB device
 * @handle: Open VirtUSB Control-Plane handle.
 * @object_id: Global runtime object ID.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_device_detach(struct virtusb_handle *handle,
                          virtusb_object_id_t object_id);

/**
 * virtusb_device_set_connected() - Control device-side USB connection signaling
 * @handle: Open VirtUSB Control-Plane handle.
 * @object_id: Global runtime object ID.
 * @connected: true to signal USB presence, false to stop signaling.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_device_set_connected(struct virtusb_handle *handle,
                                 virtusb_object_id_t object_id,
                                 bool connected);

/**
 * virtusb_set_port_power() - Set simulated downstream VBUS state
 * @handle: Open VirtUSB Control-Plane handle selecting the HCD.
 * @hub_id: Hub containing the downstream port.
 * @port: One-based downstream port number.
 * @powered: true to apply VBUS, false to remove VBUS.
 *
 * This is a simulated hardware operation, not a request to synthesize a USB
 * SetPortFeature(PORT_POWER) transaction.
 *
 * Return: 0 on success or a negative errno value on failure.
 */
int virtusb_set_port_power(struct virtusb_handle *handle,
                           uint32_t hub_id,
                           uint32_t port,
                           bool powered);

#ifdef __cplusplus
}
#endif
