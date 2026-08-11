// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/types.h>

#include "virtusb_port.h"

/**
 * DOC: VirtUSB device model
 *
 * This header defines the initial internal representation of a VirtUsbDev.
 *
 * A VirtUsbDev owns exactly one upstream VirtUsbPort. The device itself does
 * not model internal power domains, self-powered state, descriptors, or USB
 * class behavior. Those properties belong to the device implementation.
 */

/**
 * struct virtusb_device - Virtual USB device
 * @upstream_port: Upstream-facing USB port owned by this device.
 *
 * The common VirtUSB object-management base will be added when VirtUsbObjMgr is
 * implemented. The functional device representation is intentionally kept
 * minimal until then.
 */
struct virtusb_device {
   struct virtusb_port upstream_port;
};

/**
 * virtusb_device_init() - Initialize a virtual USB device
 * @device: Device to initialize.
 * @speed: Supported-speed capability mask for the upstream port.
 *
 * Return: 0 on success or a negative error code on invalid parameters.
 */
int virtusb_device_init(struct virtusb_device *device, u8 speed);

/**
 * virtusb_device_attach() - Attach a device to a downstream port
 * @device: Device to attach.
 * @downstream_port: Downstream port to attach the device to.
 *
 * Establishes the reciprocal port-to-port Attachment relationship. If the
 * effective USB-visible connection changes as a result, the owning hub's
 * C_PORT_CONNECTION state is set.
 *
 * Callers must serialize topology changes.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int virtusb_device_attach(struct virtusb_device *device,
                          struct virtusb_port *downstream_port);

/**
 * virtusb_device_detach() - Detach a virtual USB device
 * @device: Device to detach.
 *
 * Removes the reciprocal Attachment relationship. If the device was
 * USB-visible before detaching, the owning hub's C_PORT_CONNECTION state is
 * set.
 *
 * Callers must serialize topology changes.
 */
void virtusb_device_detach(struct virtusb_device *device);

/**
 * virtusb_device_set_connection_signaling() - Control device USB presence
 * @device: Device whose upstream signaling is modified.
 * @enabled: New connection-signaling state.
 *
 * This operation models the USB-facing result of a device-controller connect
 * or disconnect operation. It is independent of Attachment and VBUS.
 *
 * If the effective USB-visible connection changes, the owning hub's
 * C_PORT_CONNECTION state is set.
 *
 * Return: 0 on success or a negative error code on invalid parameters.
 */
int virtusb_device_set_connection_signaling(struct virtusb_device *device,
                                             bool enabled);

/**
 * virtusb_device_has_vbus() - Query VBUS at the device upstream port
 * @device: Device to query.
 *
 * Return: true if the device is attached to a powered downstream port,
 * otherwise false.
 */
bool virtusb_device_has_vbus(const struct virtusb_device *device);
