// SPDX-License-Identifier: GPL-2.0-only

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <virtusb_uapi.h>

#include "virtusb_control.h"
#include "virtusb_hcd.h"
#include "virtusb_device.h"
#include "virtusb_object_manager.h"
#include "virtusb_hub.h"

#define VIRTUSB_CONTROL_DEVICE_NAME "virtusb"
#define VIRTUSB_CONTROL_CLASS_NAME  "virtusb"

static dev_t virtusb_control_base_devt;
static struct class *virtusb_control_class;
static unsigned int virtusb_control_instance_count;

static bool virtusb_control_state_test(u32 state, unsigned int port)
{
   return (state & BIT(port)) != 0U;
}

static void virtusb_control_build_port_state(const struct virtusb_hub *hub,
                                             unsigned int port,
                                             struct virtusb_port_state *state)
{
   enum virtusb_port_speed speed;

   state->status = 0U;
   state->change = 0U;
   state->speed = VIRTUSB_PORT_SPEED_NONE;

   if ((port > 0U) && virtusb_hub_port_is_connected(hub, port)) {
      state->status |= VIRTUSB_PORT_STATUS_CONNECTED;
   }

   if (virtusb_control_state_test(hub->usb.enabled, port)) {
      state->status |= VIRTUSB_PORT_STATUS_ENABLED;
   }

   if (virtusb_control_state_test(hub->usb.suspended, port)) {
      state->status |= VIRTUSB_PORT_STATUS_SUSPENDED;
   }

   if (((port == 0U) && virtusb_hub_is_over_current(hub)) ||
       ((port > 0U) && virtusb_hub_port_is_over_current(hub, port))) {
      state->status |= VIRTUSB_PORT_STATUS_OVER_CURRENT;
   }

   if (virtusb_control_state_test(hub->usb.reset, port)) {
      state->status |= VIRTUSB_PORT_STATUS_RESET;
   }

   if ((port > 0U) && virtusb_hub_port_is_powered(hub, port)) {
      state->status |= VIRTUSB_PORT_STATUS_POWER;
   }

   if (virtusb_control_state_test(hub->usb_change.connected, port)) {
      state->change |= VIRTUSB_PORT_CHANGE_CONNECTED;
   }

   if (virtusb_control_state_test(hub->usb_change.enabled, port)) {
      state->change |= VIRTUSB_PORT_CHANGE_ENABLED;
   }

   if (virtusb_control_state_test(hub->usb_change.suspended, port)) {
      state->change |= VIRTUSB_PORT_CHANGE_SUSPENDED;
   }

   if (virtusb_control_state_test(hub->usb_change.over_current, port)) {
      state->change |= VIRTUSB_PORT_CHANGE_OVER_CURRENT;
   }

   if (virtusb_control_state_test(hub->usb_change.reset, port)) {
      state->change |= VIRTUSB_PORT_CHANGE_RESET;
   }

   if (port == 0U) {
      return;
   }

   speed = virtusb_hub_get_port_speed(hub, port);
   state->speed = (__u32)speed;
}

static long virtusb_control_get_port_status(struct virtusb_control *control,
                                            void __user *argp)
{
   struct virtusb_port_status request;
   const struct virtusb_hub *hub;
   unsigned int port;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   if (request.hub_id != VIRTUSB_ROOT_HUB_ID) {
      return -ENODEV;
   }

   hub = &control->hcd->root_hub.hub;

   if (request.port > hub->port_count) {
      return -EINVAL;
   }

   request.port_count = hub->port_count;

   virtusb_control_build_port_state(hub, request.port, &request.state);

   memset(request.ports, 0, sizeof(request.ports));

   if (request.port == 0U) {
      for (port = 1U; port <= hub->port_count; ++port) {
         virtusb_control_build_port_state(hub,
                                          port,
                                          &request.ports[port - 1U]);
      }
   }

   if (copy_to_user(argp, &request, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   return 0;
}

static bool virtusb_control_speed_caps_valid(__u32 speed_caps)
{
   return (speed_caps != 0U) &&
          ((speed_caps & ~VIRTUSB_SPEED_CAP_ALL) == 0U);
}

static long virtusb_control_device_create(void __user *argp)
{
   struct virtusb_device_create request;
   struct virtusb_device *device;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   if (!virtusb_control_speed_caps_valid(request.speed_caps)) {
      return -EINVAL;
   }

   device = virtusb_device_create((u8)request.speed_caps);
   if (IS_ERR(device)) {
      return PTR_ERR(device);
   }

   request.object_id = device->object.id;

   if (copy_to_user(argp, &request, sizeof(request)) != 0U) {
      (void)virtusb_device_destroy(device, true);
      virtusb_object_put(&device->object);
      return -EFAULT;
   }

   /*
    * Creation returns the object ID to userspace. The registry reference keeps
    * the device alive after this temporary creator reference is released.
    */
   virtusb_object_put(&device->object);

   return 0;
}

static long virtusb_control_device_destroy(void __user *argp)
{
   struct virtusb_device_destroy request;
   struct virtusb_object *object;
   struct virtusb_device *device;
   bool force;
   int ret;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   if ((request.object_id == VIRTUSB_OBJECT_ID_INVALID) ||
       ((request.flags & ~VIRTUSB_DEVICE_DESTROY_FLAGS) != 0U)) {
      return -EINVAL;
   }

   object = virtusb_object_lookup(request.object_id);
   if (object == NULL) {
      return -ENOENT;
   }

   if (object->type != VIRTUSB_OBJECT_TYPE_DEVICE) {
      virtusb_object_put(object);
      return -EINVAL;
   }

   device = container_of(object, struct virtusb_device, object);
   force = (request.flags & VIRTUSB_DEVICE_DESTROY_FORCE) != 0U;

   ret = virtusb_device_destroy(device, force);

   virtusb_object_put(object);

   return ret;
}

static struct virtusb_device *
virtusb_control_lookup_device(virtusb_object_id_t object_id,
                              struct virtusb_object **object)
{
   struct virtusb_object *lookup;

   if ((object == NULL) || (object_id == VIRTUSB_OBJECT_ID_INVALID)) {
      return ERR_PTR(-EINVAL);
   }

   *object = NULL;

   lookup = virtusb_object_lookup(object_id);
   if (lookup == NULL) {
      return ERR_PTR(-ENOENT);
   }

   if (lookup->type != VIRTUSB_OBJECT_TYPE_DEVICE) {
      virtusb_object_put(lookup);
      return ERR_PTR(-EINVAL);
   }

   *object = lookup;

   return container_of(lookup, struct virtusb_device, object);
}

static struct virtusb_hub *
virtusb_control_get_hub(struct virtusb_control *control, __u32 hub_id)
{
   if ((control == NULL) || (control->hcd == NULL)) {
      return NULL;
   }

   /*
    * Only the root hub is addressable at this implementation stage.
    * Hub ID 0 is the controller's root hub.
    */
   if (hub_id != 0U) {
      return NULL;
   }

   return &control->hcd->root_hub.hub;
}

static long virtusb_control_device_attach(struct virtusb_control *control,
                                          void __user *argp)
{
   struct virtusb_device_attach request;
   struct virtusb_object *object;
   struct virtusb_device *device;
   struct virtusb_hub *hub;
   struct virtusb_port *port;
   int ret;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   hub = virtusb_control_get_hub(control, request.hub_id);
   if (hub == NULL) {
      return -ENOENT;
   }

   port = virtusb_hub_get_port(hub, request.port);
   if (port == NULL) {
      return -EINVAL;
   }

   device = virtusb_control_lookup_device(request.object_id, &object);
   if (IS_ERR(device)) {
      return PTR_ERR(device);
   }

   ret = virtusb_device_attach(device, port);

   virtusb_object_put(object);

   return ret;
}

static long virtusb_control_device_detach(void __user *argp)
{
   struct virtusb_device_object request;
   struct virtusb_object *object;
   struct virtusb_device *device;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   device = virtusb_control_lookup_device(request.object_id, &object);
   if (IS_ERR(device)) {
      return PTR_ERR(device);
   }

   virtusb_device_detach(device);
   virtusb_object_put(object);

   return 0;
}

static long virtusb_control_device_connection(void __user *argp)
{
   struct virtusb_device_connection request;
   struct virtusb_object *object;
   struct virtusb_device *device;
   int ret;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   if (request.enabled > 1U) {
      return -EINVAL;
   }

   device = virtusb_control_lookup_device(request.object_id, &object);
   if (IS_ERR(device)) {
      return PTR_ERR(device);
   }

   ret = virtusb_device_set_connection_signaling(device, request.enabled != 0U);

   virtusb_object_put(object);

   return ret;
}

static long virtusb_control_set_port_power(struct virtusb_control *control,
                                           void __user *argp)
{
   struct virtusb_port_power request;
   struct virtusb_hub *hub;

   if (copy_from_user(&request, argp, sizeof(request)) != 0U) {
      return -EFAULT;
   }

   if (request.powered > 1U) {
      return -EINVAL;
   }

   hub = virtusb_control_get_hub(control, request.hub_id);
   if (hub == NULL) {
      return -ENOENT;
   }

   return virtusb_hub_set_port_power(hub,
                                     request.port,
                                     request.powered != 0U);
}

static long virtusb_control_ioctl(struct file *file,
                                  unsigned int command,
                                  unsigned long argument)
{
   struct virtusb_control *control;
   void __user *argp;

   control = file->private_data;
   if ((control == NULL) || (control->hcd == NULL) || !control->active) {
      return -ENODEV;
   }

   argp = (void __user *)argument;

   switch (command) {
   case VIRTUSB_IOCTL_GET_PORT_STATUS:
      return virtusb_control_get_port_status(control, argp);

   case VIRTUSB_IOCTL_DEVICE_CREATE:
      return virtusb_control_device_create(argp);

   case VIRTUSB_IOCTL_DEVICE_DESTROY:
      return virtusb_control_device_destroy(argp);

   case VIRTUSB_IOCTL_DEVICE_ATTACH:
      return virtusb_control_device_attach(control, argp);

   case VIRTUSB_IOCTL_DEVICE_DETACH:
      return virtusb_control_device_detach(argp);

   case VIRTUSB_IOCTL_DEVICE_CONNECTION:
      return virtusb_control_device_connection(argp);

   case VIRTUSB_IOCTL_SET_PORT_POWER:
      return virtusb_control_set_port_power(control, argp);

   default:
      return -ENOTTY;
   }
}

static int virtusb_control_open(struct inode *inode, struct file *file)
{
   struct virtusb_control *control;

   control = container_of(inode->i_cdev, struct virtusb_control, cdev);

   file->private_data = control;

   return 0;
}

static int virtusb_control_release(struct inode *inode, struct file *file)
{
   (void)inode;

   file->private_data = NULL;

   return 0;
}

static const struct file_operations virtusb_control_file_operations = {
   .owner = THIS_MODULE,
   .open = virtusb_control_open,
   .release = virtusb_control_release,
   .unlocked_ioctl = virtusb_control_ioctl,
};

int virtusb_control_register(unsigned int instance_count)
{
   int ret;

   if (instance_count == 0U) {
      return -EINVAL;
   }

   if (virtusb_control_class != NULL) {
      return -EBUSY;
   }

   ret = alloc_chrdev_region(&virtusb_control_base_devt,
                             0,
                             instance_count,
                             VIRTUSB_CONTROL_DEVICE_NAME);
   if (ret < 0) {
      return ret;
   }

   virtusb_control_class = class_create(VIRTUSB_CONTROL_CLASS_NAME);
   if (IS_ERR(virtusb_control_class)) {
      ret = PTR_ERR(virtusb_control_class);
      virtusb_control_class = NULL;

      unregister_chrdev_region(virtusb_control_base_devt, instance_count);
      virtusb_control_base_devt = 0;

      return ret;
   }

   virtusb_control_instance_count = instance_count;

   return 0;
}

void virtusb_control_unregister(void)
{
   if (virtusb_control_class == NULL) {
      return;
   }

   class_destroy(virtusb_control_class);
   virtusb_control_class = NULL;

   unregister_chrdev_region(virtusb_control_base_devt,
                            virtusb_control_instance_count);

   virtusb_control_base_devt = 0;
   virtusb_control_instance_count = 0U;
}

int virtusb_control_instance_create(struct virtusb_control *control,
                                    struct virtusb_hcd *hcd,
                                    struct device *parent,
                                    unsigned int instance)
{
   int ret;

   if ((control == NULL) || (hcd == NULL) || (parent == NULL)) {
      return -EINVAL;
   }

   if (virtusb_control_class == NULL) {
      return -ENODEV;
   }

   if (instance >= virtusb_control_instance_count) {
      return -EINVAL;
   }

   memset(control, 0, sizeof(*control));

   control->hcd = hcd;
   control->devt = MKDEV(MAJOR(virtusb_control_base_devt),
                         MINOR(virtusb_control_base_devt) + instance);

   cdev_init(&control->cdev, &virtusb_control_file_operations);
   control->cdev.owner = THIS_MODULE;

   ret = cdev_add(&control->cdev, control->devt, 1U);
   if (ret < 0) {
      goto clear_control;
   }

   control->device = device_create(virtusb_control_class,
                                   parent,
                                   control->devt,
                                   control,
                                   "virtusb%u",
                                   instance);
   if (IS_ERR(control->device)) {
      ret = PTR_ERR(control->device);
      control->device = NULL;

      goto delete_cdev;
   }

   control->active = true;

   return 0;

delete_cdev:
   cdev_del(&control->cdev);

clear_control:
   control->hcd = NULL;
   control->devt = 0;

   return ret;
}

void virtusb_control_instance_destroy(struct virtusb_control *control)
{
   if ((control == NULL) || !control->active) {
      return;
   }

   control->active = false;

   device_destroy(virtusb_control_class, control->devt);
   control->device = NULL;

   cdev_del(&control->cdev);

   control->hcd = NULL;
   control->devt = 0;
}
