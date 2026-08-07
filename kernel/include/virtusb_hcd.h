// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/types.h>

#include "virtusb_root_hub.h"

struct platform_device;

/**
 * DOC: VirtUSB host controller model
 *
 * This header defines the VirtUSB-specific state and lifecycle interface of a
 * virtual USB host controller.
 *
 * Each VirtUsbHcd represents one independent virtual USB host controller and
 * forms the root of one independent VirtUSB topology.
 *
 * Every VirtUsbHcd owns exactly one VirtUsbRHub. The root hub is represented
 * by struct virtusb_root_hub and uses the common VirtUSB hub model for its hub
 * and downstream-port state.
 *
 * The VirtUSB-specific HCD state is stored in the private data area allocated
 * by the Linux USB HCD core.
 *
 * Linux platform-driver registration is module-wide and independent of the
 * lifecycle of individual VirtUsbHcd instances. The platform driver must be
 * registered before HCD instances are created and must remain registered until
 * all instances have been destroyed.
 */

/**
 * struct virtusb_hcd - VirtUSB-specific host-controller state
 * @instance: Zero-based VirtUSB HCD instance number.
 * @root_hub: Root hub belonging to this host controller.
 *
 * @instance remains constant for the complete lifetime of the HCD.
 *
 * The containing Linux struct usb_hcd owns this structure through its private
 * HCD data area.
 */
struct virtusb_hcd {
   unsigned int instance;
   struct virtusb_root_hub root_hub;
};

/**
 * virtusb_hcd_driver_register() - Register the VirtUSB platform driver
 *
 * Registers the module-wide Linux platform driver used by all VirtUsbHcd
 * instances.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int virtusb_hcd_driver_register(void);

/**
 * virtusb_hcd_driver_unregister() - Unregister the VirtUSB platform driver
 *
 * All VirtUsbHcd instances must be destroyed before this function is called.
 */
void virtusb_hcd_driver_unregister(void);

/**
 * virtusb_hcd_create() - Create a virtual USB host-controller instance
 * @instance: Zero-based VirtUSB HCD instance number.
 * @port_count: Number of downstream ports provided by the root hub.
 *
 * Creates and registers a Linux platform device. The registered VirtUSB
 * platform driver probes the device, creates the corresponding Linux USB HCD,
 * initializes the VirtUSB-specific HCD state, and registers the controller with
 * the Linux USB core.
 *
 * virtusb_hcd_driver_register() must have completed successfully before this
 * function is called.
 *
 * Return: Pointer to the Linux platform device on success or an ERR_PTR()
 * encoded error value on failure.
 */
struct platform_device *virtusb_hcd_create(unsigned int instance,
                                           unsigned int port_count);

/**
 * virtusb_hcd_destroy() - Destroy a virtual USB host-controller instance
 * @pdev: Linux platform device returned by virtusb_hcd_create().
 *
 * Unregisters the platform device. The platform driver's remove callback
 * unregisters and releases the associated Linux USB HCD.
 *
 * Passing NULL has no effect.
 */
void virtusb_hcd_destroy(struct platform_device *pdev);
