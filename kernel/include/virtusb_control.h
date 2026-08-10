// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/cdev.h>
#include <linux/types.h>

struct device;
struct virtusb_hcd;

/**
 * DOC: VirtUSB user-space Control Plane
 *
 * This header defines the kernel-side lifecycle of the VirtUSB character-device
 * Control Plane.
 *
 * The Control Plane provides one character device per VirtUsbHcd instance:
 *
 * /dev/virtusb0 -> VirtUsbHcd instance 0
 * /dev/virtusb1 -> VirtUsbHcd instance 1
 * ...
 *
 * Module-wide resources such as the character-device number range and device
 * class are registered once. Each VirtUsbHcd then owns one
 * struct virtusb_control instance for its corresponding character device.
 *
 * The Control Plane currently provides open(), release(), and read-only ioctl
 * access through VIRTUSB_IOCTL_GET_PORT_STATUS. Additional UAPI commands are
 * added separately as their corresponding state transitions are implemented.
 */

/**
 * struct virtusb_control - Control-Plane state of one VirtUsbHcd
 * @hcd: VirtUsbHcd owning this Control-Plane instance.
 * @cdev: Linux character-device object.
 * @device: Linux device representing /dev/virtusbX.
 * @devt: Character-device number assigned to this instance.
 * @active: True while the character-device instance is registered.
 */
struct virtusb_control {
   struct virtusb_hcd *hcd;
   struct cdev cdev;
   struct device *device;
   dev_t devt;
   bool active;
};

/**
 * virtusb_control_register() - Register module-wide Control-Plane resources
 * @instance_count: Number of VirtUsbHcd instances that may be exposed.
 *
 * Allocates a contiguous character-device number range with one minor number
 * per VirtUsbHcd instance and creates the VirtUSB device class.
 *
 * This function must complete successfully before any Control-Plane instance
 * is created.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int virtusb_control_register(unsigned int instance_count);

/**
 * virtusb_control_unregister() - Unregister module-wide Control-Plane resources
 *
 * All Control-Plane instances must be destroyed before this function is
 * called.
 */
void virtusb_control_unregister(void);

/**
 * virtusb_control_instance_create() - Create one VirtUSB character device
 * @control: Control-Plane state to initialize.
 * @hcd: VirtUsbHcd owning the Control-Plane instance.
 * @parent: Linux device used as the parent of the character device.
 * @instance: Zero-based VirtUsbHcd instance number.
 *
 * Creates /dev/virtusbX where X equals @instance. The instance number maps
 * directly to the allocated character-device minor number.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int virtusb_control_instance_create(struct virtusb_control *control,
                                    struct virtusb_hcd *hcd,
                                    struct device *parent,
                                    unsigned int instance);

/**
 * virtusb_control_instance_destroy() - Destroy one VirtUSB character device
 * @control: Control-Plane instance to destroy.
 *
 * Passing NULL or an inactive instance has no effect.
 */
void virtusb_control_instance_destroy(struct virtusb_control *control);
