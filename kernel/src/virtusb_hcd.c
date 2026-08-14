// SPDX-License-Identifier: GPL-2.0-only

#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/unaligned.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include "virtusb_hcd.h"

#define VIRTUSB_HCD_DEVICE_NAME "virtusb-hcd"
#define VIRTUSB_HCD_DESCRIPTION "virtusb-hcd"
#define VIRTUSB_HCD_PRODUCT     "VirtUSB Host Controller"

static struct virtusb_hcd *virtusb_hcd_from_linux(struct usb_hcd *hcd)
{
   return (struct virtusb_hcd *)hcd->hcd_priv;
}

static bool virtusb_hcd_port_state_test(u32 state, unsigned int port_number)
{
   return (state & BIT(port_number)) != 0U;
}

static void virtusb_hcd_port_state_set(u32 *state, unsigned int port_number)
{
   *state |= BIT(port_number);
}

static void virtusb_hcd_port_state_clear(u32 *state, unsigned int port_number)
{
   *state &= ~BIT(port_number);
}

static u32 virtusb_hcd_port_change_mask(const struct virtusb_hub *hub)
{
   u32 changes;

   changes = hub->usb_change.connected |
             hub->usb_change.enabled |
             hub->usb_change.suspended |
             hub->usb_change.reset |
             hub->usb_change.over_current;

   /*
    * Bit 0 represents hub-level change state. Currently only over-current
    * has meaningful hub-level semantics.
    */
   changes &= ~BIT(0);
   changes |= hub->usb_change.over_current & BIT(0);

   return changes;
}

static u16 virtusb_hcd_build_port_status(const struct virtusb_hub *hub,
                                         unsigned int port_number)
{
   enum virtusb_port_speed speed;
   u16 status = 0U;

   if (virtusb_hub_port_is_connected(hub, port_number)) {
      status |= USB_PORT_STAT_CONNECTION;
   }

   if (virtusb_hcd_port_state_test(hub->usb.enabled, port_number)) {
      status |= USB_PORT_STAT_ENABLE;
   }

   if (virtusb_hcd_port_state_test(hub->usb.suspended, port_number)) {
      status |= USB_PORT_STAT_SUSPEND;
   }

   if (virtusb_hub_port_is_over_current(hub, port_number)) {
      status |= USB_PORT_STAT_OVERCURRENT;
   }

   if (virtusb_hcd_port_state_test(hub->usb.reset, port_number)) {
      status |= USB_PORT_STAT_RESET;
   }

   if (virtusb_hub_port_is_powered(hub, port_number)) {
      status |= USB_PORT_STAT_POWER;
   }

   speed = virtusb_hub_get_port_speed(hub, port_number);

   switch (speed) {
   case VIRTUSB_PORT_SPEED_LOW:
      status |= USB_PORT_STAT_LOW_SPEED;
      break;

   case VIRTUSB_PORT_SPEED_HIGH:
      status |= USB_PORT_STAT_HIGH_SPEED;
      break;

   case VIRTUSB_PORT_SPEED_NONE:
   case VIRTUSB_PORT_SPEED_FULL:
      break;
   }

   return status;
}

static u16 virtusb_hcd_build_port_change(const struct virtusb_hub *hub,
                                         unsigned int port_number)
{
   u16 change = 0U;

   if (virtusb_hcd_port_state_test(hub->usb_change.connected, port_number)) {
      change |= USB_PORT_STAT_C_CONNECTION;
   }

   if (virtusb_hcd_port_state_test(hub->usb_change.enabled, port_number)) {
      change |= USB_PORT_STAT_C_ENABLE;
   }

   if (virtusb_hcd_port_state_test(hub->usb_change.suspended, port_number)) {
      change |= USB_PORT_STAT_C_SUSPEND;
   }

   if (virtusb_hcd_port_state_test(hub->usb_change.over_current, port_number)) {
      change |= USB_PORT_STAT_C_OVERCURRENT;
   }

   if (virtusb_hcd_port_state_test(hub->usb_change.reset, port_number)) {
      change |= USB_PORT_STAT_C_RESET;
   }

   return change;
}

static void virtusb_hcd_root_hub_status_changed(struct virtusb_hub *hub,
                                                  void *context)
{
   struct usb_hcd *hcd = context;

   (void)hub;

   if (hcd != NULL) {
      usb_hcd_poll_rh_status(hcd);
   }
}

static int virtusb_hcd_start(struct usb_hcd *hcd)
{
   /*
    * VirtUSB has no physical controller hardware to start. Root-hub changes
    * are reported explicitly through usb_hcd_poll_rh_status().
    */
   hcd->uses_new_polling = 1;

   return 0;
}

static void virtusb_hcd_stop(struct usb_hcd *hcd)
{
   (void)hcd;
}

static int virtusb_hcd_urb_enqueue(struct usb_hcd *hcd,
                                   struct urb *urb,
                                   gfp_t mem_flags)
{
   (void)hcd;
   (void)urb;
   (void)mem_flags;

   /*
    * No downstream VirtUsbDev transfer path exists yet.
    */
   return -ENODEV;
}

static int virtusb_hcd_urb_dequeue(struct usb_hcd *hcd,
                                   struct urb *urb,
                                   int status)
{
   (void)hcd;
   (void)urb;
   (void)status;

   /*
    * No downstream URBs can be queued yet.
    */
   return -ENODEV;
}

static int virtusb_hcd_hub_status_data(struct usb_hcd *hcd, char *buf)
{
   struct virtusb_hcd *virt_hcd;
   struct virtusb_hub *hub;
   unsigned int byte_count;
   unsigned int i;
   u32 changes;

   virt_hcd = virtusb_hcd_from_linux(hcd);
   hub = &virt_hcd->root_hub.hub;

   changes = virtusb_hcd_port_change_mask(hub);
   if (changes == 0U) {
      return 0;
   }

   byte_count = (hub->port_count + 8U) / 8U;

   for (i = 0U; i < byte_count; ++i) {
      buf[i] = (char)((changes >> (i * 8U)) & 0xffU);
   }

   return (int)byte_count;
}

static int virtusb_hcd_get_hub_descriptor(const struct virtusb_hub *hub, char *buf)
{
   unsigned int bitmap_bytes;
   unsigned int descriptor_length;
   u8 *descriptor;

   descriptor = (u8 *)buf;

   /*
    * USB 2.0 hub descriptors contain two variable-size bitmaps after the
    * seven-byte fixed part: DeviceRemovable and PortPwrCtrlMask.
    */
   bitmap_bytes = (hub->port_count + 8U) / 8U;
   descriptor_length = USB_DT_HUB_NONVAR_SIZE + (2U * bitmap_bytes);

   memset(descriptor, 0, descriptor_length);

   descriptor[0] = (u8)descriptor_length;
   descriptor[1] = USB_DT_HUB;
   descriptor[2] = hub->port_count;

   {
      u16 characteristics = 0U;

      if (hub->power_switching_mode == VIRTUSB_HUB_POWER_SWITCHING_INDIVIDUAL) {
         characteristics |= HUB_CHAR_INDV_PORT_LPSM;
      }

      switch (hub->over_current_mode) {
      case VIRTUSB_HUB_OVER_CURRENT_PER_PORT:
         characteristics |= HUB_CHAR_INDV_PORT_OCPM;
         break;

      case VIRTUSB_HUB_OVER_CURRENT_NONE:
         characteristics |= HUB_CHAR_NO_OCPM;
         break;

      case VIRTUSB_HUB_OVER_CURRENT_GLOBAL:
         break;
      }

      put_unaligned_le16(characteristics, &descriptor[3]);
   }

   descriptor[5] = 0U;
   descriptor[6] = 0U;

   /*
    * All downstream ports are removable.
    */
   memset(&descriptor[USB_DT_HUB_NONVAR_SIZE], 0, bitmap_bytes);

   /*
    * Port power control is available for all downstream ports.
    */
   memset(&descriptor[USB_DT_HUB_NONVAR_SIZE + bitmap_bytes], 0xff, bitmap_bytes);

   return (int)descriptor_length;
}

static int virtusb_hcd_get_hub_status(const struct virtusb_hub *hub, char *buf)
{
   u16 status = 0U;
   u16 change = 0U;

   if (virtusb_hub_is_over_current(hub)) {
      status |= HUB_STATUS_OVERCURRENT;
   }

   if (virtusb_hcd_port_state_test(hub->usb_change.over_current, 0U)) {
      change |= HUB_CHANGE_OVERCURRENT;
   }

   put_unaligned_le16(status, &buf[0]);
   put_unaligned_le16(change, &buf[2]);

   return 0;
}

static int virtusb_hcd_get_port_status(const struct virtusb_hub *hub,
                                       unsigned int port_number,
                                       char *buf)
{
   u16 status;
   u16 change;

   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return -EPIPE;
   }

   status = virtusb_hcd_build_port_status(hub, port_number);
   change = virtusb_hcd_build_port_change(hub, port_number);

   put_unaligned_le16(status, &buf[0]);
   put_unaligned_le16(change, &buf[2]);

   return 0;
}

static int virtusb_hcd_clear_port_feature(struct virtusb_hub *hub,
                                          unsigned int port_number,
                                          u16 feature)
{
   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return -EPIPE;
   }

   switch (feature) {
   case USB_PORT_FEAT_ENABLE:
      virtusb_hcd_port_state_clear(&hub->usb.enabled, port_number);
      break;

   case USB_PORT_FEAT_SUSPEND:
      /*
       * Clearing PORT_SUSPEND starts host-initiated resume processing.
       *
       * VirtUSB does not model resume timing yet. Until that state machine
       * exists, clear the USB-visible suspend state directly.
       */
      virtusb_hcd_port_state_clear(&hub->usb.suspended, port_number);
      break;

   case USB_PORT_FEAT_POWER:
      return virtusb_hub_set_port_power(hub, port_number, false);

   case USB_PORT_FEAT_C_CONNECTION:
      virtusb_hcd_port_state_clear(&hub->usb_change.connected, port_number);
      break;

   case USB_PORT_FEAT_C_ENABLE:
      virtusb_hcd_port_state_clear(&hub->usb_change.enabled, port_number);
      break;

   case USB_PORT_FEAT_C_SUSPEND:
      virtusb_hcd_port_state_clear(&hub->usb_change.suspended, port_number);
      break;

   case USB_PORT_FEAT_C_OVER_CURRENT:
      virtusb_hcd_port_state_clear(&hub->usb_change.over_current, port_number);
      break;

   case USB_PORT_FEAT_C_RESET:
      virtusb_hcd_port_state_clear(&hub->usb_change.reset, port_number);
      break;

   default:
      return -EPIPE;
   }

   return 0;
}

static int virtusb_hcd_set_port_feature(struct virtusb_hub *hub,
                                        unsigned int port_number,
                                        u16 feature)
{
   if (!virtusb_hub_port_is_valid(hub, port_number)) {
      return -EPIPE;
   }

   switch (feature) {
   case USB_PORT_FEAT_SUSPEND:
      /*
       * PORT_SUSPEND is a USB-controlled state. It does not modify the
       * independently simulated hardware state.
       */
      virtusb_hcd_port_state_set(&hub->usb.suspended, port_number);
      break;

   case USB_PORT_FEAT_POWER:
      return virtusb_hub_set_port_power(hub, port_number, true);

   case USB_PORT_FEAT_RESET:
      /*
       * A reset of an unconnected port has no effect.
       *
       * Reset timing and completion will be implemented together with the
       * downstream-device connection model. Completion must eventually clear
       * PORT_RESET, set PORT_ENABLE, and set C_PORT_RESET.
       */
      if (virtusb_hub_port_is_connected(hub, port_number)) {
         virtusb_hcd_port_state_clear(&hub->usb.enabled, port_number);
         virtusb_hcd_port_state_set(&hub->usb.reset, port_number);
      }
      break;

   default:
      return -EPIPE;
   }

   return 0;
}

static int virtusb_hcd_hub_control(struct usb_hcd *hcd,
                                   u16 type_req,
                                   u16 value,
                                   u16 index,
                                   char *buf,
                                   u16 length)
{
   struct virtusb_hcd *virt_hcd;
   struct virtusb_hub *hub;
   unsigned int port_number;

   (void)length;

   virt_hcd = virtusb_hcd_from_linux(hcd);
   hub = &virt_hcd->root_hub.hub;
   port_number = index & 0xffU;

   switch (type_req) {
   case ClearHubFeature:
      switch (value) {
      case C_HUB_LOCAL_POWER:
         /*
          * Local-power change state is not modeled yet.
          */
         return 0;

      case C_HUB_OVER_CURRENT:
         virtusb_hcd_port_state_clear(&hub->usb_change.over_current, 0U);
         return 0;

      default:
         return -EPIPE;
      }

   case ClearPortFeature:
      return virtusb_hcd_clear_port_feature(hub, port_number, value);

   case GetHubDescriptor:
      return virtusb_hcd_get_hub_descriptor(hub, buf);

   case GetHubStatus:
      return virtusb_hcd_get_hub_status(hub, buf);

   case GetPortStatus:
      return virtusb_hcd_get_port_status(hub, port_number, buf);

   case SetHubFeature:
      return -EPIPE;

   case SetPortFeature:
      return virtusb_hcd_set_port_feature(hub, port_number, value);

   default:
      return -EPIPE;
   }
}

static const struct hc_driver virtusb_hc_driver = {
   .description = VIRTUSB_HCD_DESCRIPTION,
   .product_desc = VIRTUSB_HCD_PRODUCT,
   .hcd_priv_size = sizeof(struct virtusb_hcd),
   .flags = HCD_USB2,

   .start = virtusb_hcd_start,
   .stop = virtusb_hcd_stop,

   .urb_enqueue = virtusb_hcd_urb_enqueue,
   .urb_dequeue = virtusb_hcd_urb_dequeue,

   .hub_status_data = virtusb_hcd_hub_status_data,
   .hub_control = virtusb_hcd_hub_control,
};

static int virtusb_hcd_platform_probe(struct platform_device *pdev)
{
   const unsigned int *port_count;
   struct virtusb_hcd *virt_hcd;
   struct usb_hcd *hcd;
   int ret;

   port_count = dev_get_platdata(&pdev->dev);
   if (port_count == NULL) {
      return -EINVAL;
   }

   if ((*port_count == 0U) || (*port_count > VIRTUSB_MAX_HUB_PORTS)) {
      return -EINVAL;
   }

   hcd = usb_create_hcd(&virtusb_hc_driver, &pdev->dev, dev_name(&pdev->dev));
   if (hcd == NULL) {
      return -ENOMEM;
   }

   virt_hcd = virtusb_hcd_from_linux(hcd);
   virt_hcd->instance = (unsigned int)pdev->id;

   ret = virtusb_root_hub_init(&virt_hcd->root_hub, *port_count);
   if (ret < 0) {
      goto put_hcd;
   }

   virtusb_hub_set_status_changed_callback(
      &virt_hcd->root_hub.hub,
      virtusb_hcd_root_hub_status_changed,
      hcd);

   platform_set_drvdata(pdev, hcd);

   /*
    * VirtUSB has no physical IRQ. Root-hub changes are reported through the
    * generic root-hub polling interface.
    */
   ret = usb_add_hcd(hcd, 0, 0);
   if (ret < 0) {
      goto clear_drvdata;
   }

   ret = virtusb_control_instance_create(&virt_hcd->control,
                                         virt_hcd,
                                         &pdev->dev,
                                         virt_hcd->instance);
   if (ret < 0) {
      goto remove_hcd;
   }

   return 0;

remove_hcd:
   usb_remove_hcd(hcd);

clear_drvdata:
   platform_set_drvdata(pdev, NULL);

put_hcd:
   usb_put_hcd(hcd);

   return ret;
}

static void virtusb_hcd_platform_remove(struct platform_device *pdev)
{
   struct virtusb_hcd *virt_hcd;
   struct usb_hcd *hcd;

   hcd = platform_get_drvdata(pdev);
   if (hcd == NULL) {
      return;
   }

   virt_hcd = virtusb_hcd_from_linux(hcd);

   virtusb_control_instance_destroy(&virt_hcd->control);
   virtusb_hub_set_status_changed_callback(&virt_hcd->root_hub.hub, NULL, NULL);
   usb_remove_hcd(hcd);
   platform_set_drvdata(pdev, NULL);
   usb_put_hcd(hcd);
}

static struct platform_driver virtusb_hcd_platform_driver = {
   .probe = virtusb_hcd_platform_probe,
   .remove = virtusb_hcd_platform_remove,
   .driver = {
      .name = VIRTUSB_HCD_DEVICE_NAME,
   },
};

int virtusb_hcd_driver_register(void)
{
   return platform_driver_register(&virtusb_hcd_platform_driver);
}

void virtusb_hcd_driver_unregister(void)
{
   platform_driver_unregister(&virtusb_hcd_platform_driver);
}

struct platform_device *virtusb_hcd_create(unsigned int instance,
                                           unsigned int port_count)
{
   struct platform_device *pdev;
   struct usb_hcd *hcd;
   int ret;

   if ((port_count == 0U) || (port_count > VIRTUSB_MAX_HUB_PORTS)) {
      return ERR_PTR(-EINVAL);
   }

   pdev = platform_device_alloc(VIRTUSB_HCD_DEVICE_NAME, (int)instance);
   if (pdev == NULL) {
      return ERR_PTR(-ENOMEM);
   }

   ret = platform_device_add_data(pdev, &port_count, sizeof(port_count));
   if (ret < 0) {
      goto put_device;
   }

   ret = platform_device_add(pdev);
   if (ret < 0) {
      goto put_device;
   }

   /*
    * platform_device_add() registers the device even if probe() fails.
    * A successfully bound VirtUSB HCD always stores its struct usb_hcd in
    * driver data, so verify the binding before returning the instance.
    */
   hcd = platform_get_drvdata(pdev);
   if (hcd == NULL) {
      platform_device_unregister(pdev);
      return ERR_PTR(-ENODEV);
   }

   return pdev;

put_device:
   platform_device_put(pdev);

   return ERR_PTR(ret);
}

void virtusb_hcd_destroy(struct platform_device *pdev)
{
   if (pdev == NULL) {
      return;
   }

   /*
    * Unregistering the platform device invokes the bound driver's remove()
    * callback, which removes and releases the Linux USB HCD.
    */
   platform_device_unregister(pdev);
}
