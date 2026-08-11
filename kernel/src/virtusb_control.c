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
