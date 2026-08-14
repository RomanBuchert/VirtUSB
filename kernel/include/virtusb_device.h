// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/types.h>

#include "virtusb_object.h"
#include "virtusb_port.h"

/**
 * DOC: VirtUSB device model
 *
 * A VirtUsbDev is a reference-counted VirtUSB Core Component registered with
 * VirtUsbObjMgr and owns exactly one upstream VirtUsbPort.
 *
 * Object existence remains independent of Attachment and USB-visible state.
 * The device itself does not model internal power domains, self-powered state,
 * descriptors, or USB class behavior. Those properties belong to the device
 * implementation.
 */

/**
 * struct virtusb_device - Virtual USB device
 * @object: Common VirtUSB object identity and lifetime state.
 * @upstream_port: Upstream-facing USB port owned by this device.
 *
 * @object is the first member so that the generic object layer can allocate
 * storage for the complete concrete component.
 */
struct virtusb_device {
   struct virtusb_object object;
   struct virtusb_port upstream_port;
};

/**
 * virtusb_device_create() - Create and publish a virtual USB device
 * @speed: Supported-speed capability mask for the upstream port.
 *
 * Allocates the complete device object, initializes its device-specific state,
 * and registers it with VirtUsbObjMgr as VIRTUSB_OBJECT_TYPE_DEVICE.
 *
 * The returned pointer owns one caller reference in addition to the registry
 * reference. The caller must eventually release that reference, either with
 * virtusb_object_put() or by passing it to virtusb_device_destroy().
 *
 * Return: Referenced device on success or an ERR_PTR()-encoded error.
 */
struct virtusb_device *virtusb_device_create(u8 speed);

/**
 * virtusb_device_destroy() - Destroy a virtual USB device
 * @device: Device to destroy.
 * @force: Perform type-specific forced cleanup before destruction.
 *
 * Normal destruction is conservative and fails with -EBUSY while the device
 * remains attached. Forced destruction first disables connection signaling
 * and detaches the device.
 *
 * This function unregisters the device but does not consume the caller's
 * reference.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int virtusb_device_destroy(struct virtusb_device *device, bool force);

/**
 * virtusb_device_shutdown_all() - Force shutdown of all remaining devices
 *
 * Used by the module shutdown path before HCD infrastructure is destroyed.
 * Every published VirtUsbDev is forcibly disconnected, detached, and
 * unregistered.
 */
void virtusb_device_shutdown_all(void);

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
