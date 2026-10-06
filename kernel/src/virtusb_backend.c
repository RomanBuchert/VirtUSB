// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include <virtusb_uapi.h>

#include "virtusb_backend.h"
#include "virtusb_device.h"
#include "virtusb_hcd.h"
#include "virtusb_object.h"
#include "virtusb_transfer.h"

static void virtusb_backend_release_request(struct virtusb_hcd_request *request)
{
   virtusb_transfer_put(request->transfer);
   kfree(request);
}

static void virtusb_backend_giveback(struct usb_hcd *hcd,
                                     struct virtusb_hcd_request *request,
                                     int status)
{
   usb_hcd_giveback_urb(hcd, request->urb, status);
   virtusb_backend_release_request(request);
}

void virtusb_backend_init(struct virtusb_backend *backend)
{
   spin_lock_init(&backend->lock);
   INIT_LIST_HEAD(&backend->bindings);
   INIT_LIST_HEAD(&backend->pending);
   INIT_LIST_HEAD(&backend->active);
   init_waitqueue_head(&backend->wait_queue);
   backend->stopping = false;
}

static struct virtusb_backend_binding *virtusb_backend_find_owner_locked(
   struct virtusb_backend *backend,
   struct file *owner)
{
   struct virtusb_backend_binding *binding;

   list_for_each_entry(binding, &backend->bindings, node) {
      if (binding->owner == owner) {
         return binding;
      }
   }

   return NULL;
}

static struct virtusb_backend_binding *virtusb_backend_find_device_locked(
   struct virtusb_backend *backend,
   struct virtusb_device *device)
{
   struct virtusb_backend_binding *binding;

   list_for_each_entry(binding, &backend->bindings, node) {
      if (binding->device == device) {
         return binding;
      }
   }

   return NULL;
}

int virtusb_backend_register(struct virtusb_hcd *virt_hcd,
                             struct file *owner,
                             struct virtusb_device *device)
{
   struct virtusb_backend_binding *binding;
   unsigned long flags;
   int ret = 0;

   if ((virt_hcd == NULL) || (owner == NULL) || (device == NULL)) {
      return -EINVAL;
   }

   binding = kzalloc(sizeof(*binding), GFP_KERNEL);
   if (binding == NULL) {
      return -ENOMEM;
   }

   binding->device = device;
   binding->owner = owner;
   INIT_LIST_HEAD(&binding->node);
   virtusb_object_get(&device->object);

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);
   if (virt_hcd->backend.stopping) {
      ret = -ESHUTDOWN;
   } else if ((virtusb_backend_find_owner_locked(&virt_hcd->backend, owner) != NULL) ||
              (virtusb_backend_find_device_locked(&virt_hcd->backend, device) != NULL)) {
      ret = -EBUSY;
   } else {
      list_add_tail(&binding->node, &virt_hcd->backend.bindings);
   }
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

   if (ret < 0) {
      virtusb_object_put(&device->object);
      kfree(binding);
      return ret;
   }

   wake_up_interruptible_all(&virt_hcd->backend.wait_queue);
   return 0;
}

static struct virtusb_hcd_request *virtusb_backend_find_urb_locked(
   struct virtusb_backend *backend,
   struct urb *urb)
{
   struct virtusb_hcd_request *request;

   list_for_each_entry(request, &backend->pending, node) {
      if (request->urb == urb) {
         return request;
      }
   }

   list_for_each_entry(request, &backend->active, node) {
      if (request->urb == urb) {
         return request;
      }
   }

   return NULL;
}

static struct virtusb_hcd_request *virtusb_backend_find_id_locked(
   struct virtusb_backend *backend,
   struct file *owner,
   u64 transfer_id)
{
   struct virtusb_hcd_request *request;

   list_for_each_entry(request, &backend->active, node) {
      if ((request->owner == owner) && (request->transfer->id == transfer_id)) {
         return request;
      }
   }

   return NULL;
}

static struct virtusb_hcd_request *virtusb_backend_find_pending_device_locked(
   struct virtusb_backend *backend,
   struct virtusb_device *device)
{
   struct virtusb_hcd_request *request;

   list_for_each_entry(request, &backend->pending, node) {
      if (request->transfer->device == device) {
         return request;
      }
   }

   return NULL;
}

int virtusb_backend_submit(struct virtusb_hcd *virt_hcd,
                           struct usb_hcd *hcd,
                           struct urb *urb,
                           struct virtusb_transfer *transfer,
                           gfp_t mem_flags)
{
   struct virtusb_hcd_request *request;
   unsigned long flags;
   int ret;

   if ((virt_hcd == NULL) || (hcd == NULL) || (urb == NULL) ||
       (transfer == NULL)) {
      return -EINVAL;
   }

   if (transfer->requested_length > VIRTUSB_TRANSFER_DATA_MAX) {
      return -EMSGSIZE;
   }

   request = kzalloc(sizeof(*request), mem_flags);
   if (request == NULL) {
      return -ENOMEM;
   }

   request->urb = urb;
   request->transfer = virtusb_transfer_get(transfer);
   INIT_LIST_HEAD(&request->node);

   ret = virtusb_transfer_mark_queued(transfer);
   if (ret < 0) {
      virtusb_backend_release_request(request);
      return ret;
   }

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);
   if (virt_hcd->backend.stopping) {
      ret = -ESHUTDOWN;
   } else if (virtusb_backend_find_device_locked(&virt_hcd->backend,
                                                  transfer->device) == NULL) {
      ret = -ENODEV;
   } else {
      ret = usb_hcd_link_urb_to_ep(hcd, urb);
      if (ret == 0) {
         list_add_tail(&request->node, &virt_hcd->backend.pending);
      }
   }
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

   if (ret < 0) {
      (void)virtusb_transfer_cancel(transfer, ret);
      virtusb_backend_release_request(request);
      return ret;
   }

   wake_up_interruptible_all(&virt_hcd->backend.wait_queue);
   return 0;
}

int virtusb_backend_cancel(struct virtusb_hcd *virt_hcd,
                           struct usb_hcd *hcd,
                           struct urb *urb,
                           int status)
{
   struct virtusb_hcd_request *request;
   unsigned long flags;
   int ret;

   if ((virt_hcd == NULL) || (hcd == NULL) || (urb == NULL)) {
      return -EINVAL;
   }

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);

   ret = usb_hcd_check_unlink_urb(hcd, urb, status);
   if (ret < 0) {
      spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);
      return ret;
   }

   request = virtusb_backend_find_urb_locked(&virt_hcd->backend, urb);
   if (request == NULL) {
      spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);
      return -EIDRM;
   }

   list_del_init(&request->node);
   usb_hcd_unlink_urb_from_ep(hcd, urb);
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

   (void)virtusb_transfer_cancel(request->transfer, status);
   virtusb_backend_giveback(hcd, request, status);
   return 0;
}

static bool virtusb_backend_pending_for_owner_or_stopping(
   struct virtusb_backend *backend,
   struct file *owner)
{
   struct virtusb_backend_binding *binding;
   unsigned long flags;
   bool ready;

   spin_lock_irqsave(&backend->lock, flags);
   binding = virtusb_backend_find_owner_locked(backend, owner);
   ready = backend->stopping ||
           ((binding != NULL) &&
            (virtusb_backend_find_pending_device_locked(backend, binding->device) != NULL));
   spin_unlock_irqrestore(&backend->lock, flags);

   return ready;
}

bool virtusb_backend_has_pending(struct virtusb_hcd *virt_hcd, struct file *owner)
{
   if ((virt_hcd == NULL) || (owner == NULL)) {
      return false;
   }

   return virtusb_backend_pending_for_owner_or_stopping(&virt_hcd->backend, owner);
}

int virtusb_backend_fetch(struct virtusb_hcd *virt_hcd,
                          struct file *owner,
                          struct virtusb_transfer_fetch *fetch,
                          bool nonblock)
{
   struct virtusb_backend_binding *binding;
   struct virtusb_hcd_request *request;
   struct virtusb_transfer *transfer;
   unsigned long flags;
   int ret;

   if ((virt_hcd == NULL) || (owner == NULL) || (fetch == NULL)) {
      return -EINVAL;
   }

   for (;;) {
      spin_lock_irqsave(&virt_hcd->backend.lock, flags);

      binding = virtusb_backend_find_owner_locked(&virt_hcd->backend, owner);
      if (binding == NULL) {
         spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);
         return -ENODEV;
      }

      request = virtusb_backend_find_pending_device_locked(&virt_hcd->backend,
                                                            binding->device);
      if (request != NULL) {
         request->owner = owner;
         list_move_tail(&request->node, &virt_hcd->backend.active);
         transfer = virtusb_transfer_get(request->transfer);
         spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);
         break;
      }

      if (virt_hcd->backend.stopping) {
         spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);
         return -ESHUTDOWN;
      }

      spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

      if (nonblock) {
         return -EAGAIN;
      }

      ret = wait_event_interruptible(
         virt_hcd->backend.wait_queue,
         virtusb_backend_pending_for_owner_or_stopping(&virt_hcd->backend, owner));
      if (ret < 0) {
         return ret;
      }
   }

   memset(fetch, 0, sizeof(*fetch));
   fetch->transfer_id = transfer->id;
   fetch->object_id = transfer->device->object.id;
   fetch->type = (__u32)transfer->type;
   fetch->direction = (__u32)transfer->direction;
   fetch->endpoint = transfer->endpoint;
   fetch->requested_length = (__u32)transfer->requested_length;

   if (transfer->has_setup) {
      fetch->has_setup = 1U;
      memcpy(fetch->setup, &transfer->setup, sizeof(fetch->setup));
   }

   if ((transfer->direction == VIRTUSB_TRANSFER_DIRECTION_OUT) &&
       (transfer->requested_length > 0U)) {
      memcpy(fetch->data, transfer->buffer, transfer->requested_length);
      fetch->data_length = (__u32)transfer->requested_length;
   }

   virtusb_transfer_put(transfer);
   return 0;
}

int virtusb_backend_fetch_rollback(struct virtusb_hcd *virt_hcd,
                                   struct file *owner,
                                   u64 transfer_id)
{
   struct virtusb_hcd_request *request;
   unsigned long flags;

   if ((virt_hcd == NULL) || (owner == NULL) || (transfer_id == 0U)) {
      return -EINVAL;
   }

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);
   request = virtusb_backend_find_id_locked(&virt_hcd->backend, owner, transfer_id);
   if (request != NULL) {
      request->owner = NULL;
      list_move(&request->node, &virt_hcd->backend.pending);
   }
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

   if (request == NULL) {
      return -ENOENT;
   }

   wake_up_interruptible_all(&virt_hcd->backend.wait_queue);
   return 0;
}

int virtusb_backend_complete(struct virtusb_hcd *virt_hcd,
                             struct usb_hcd *hcd,
                             struct file *owner,
                             const struct virtusb_transfer_complete_request *completion)
{
   struct virtusb_hcd_request *request;
   struct virtusb_transfer *transfer;
   unsigned long flags;
   size_t actual_length;
   int ret;

   if ((virt_hcd == NULL) || (hcd == NULL) || (owner == NULL) ||
       (completion == NULL) || (completion->transfer_id == 0U)) {
      return -EINVAL;
   }

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);
   request = virtusb_backend_find_id_locked(&virt_hcd->backend,
                                             owner,
                                             completion->transfer_id);
   if (request != NULL) {
      list_del_init(&request->node);
      usb_hcd_unlink_urb_from_ep(hcd, request->urb);
   }
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

   if (request == NULL) {
      return -ENOENT;
   }

   transfer = request->transfer;
   actual_length = completion->actual_length;

   if ((actual_length > transfer->requested_length) ||
       (actual_length > sizeof(completion->data))) {
      (void)virtusb_transfer_cancel(transfer, -EMSGSIZE);
      virtusb_backend_giveback(hcd, request, -EMSGSIZE);
      return -EMSGSIZE;
   }

   /*
    * actual_length may legitimately be non-zero together with an error status.
    * Preserve partial IN data whenever the backend reports transferred bytes.
    */
   if ((transfer->direction == VIRTUSB_TRANSFER_DIRECTION_IN) &&
       (actual_length > 0U)) {
      memcpy(transfer->buffer, completion->data, actual_length);
      memcpy(request->urb->transfer_buffer, transfer->buffer, actual_length);
   }

   ret = virtusb_transfer_complete(transfer, completion->status, actual_length);
   if (ret < 0) {
      (void)virtusb_transfer_cancel(transfer, ret);
      virtusb_backend_giveback(hcd, request, ret);
      return ret;
   }

   request->urb->actual_length = (unsigned int)actual_length;
   virtusb_backend_giveback(hcd, request, completion->status);
   return 0;
}

void virtusb_backend_release_owner(struct virtusb_hcd *virt_hcd,
                                   struct usb_hcd *hcd,
                                   struct file *owner)
{
   struct virtusb_backend_binding *binding;
   struct virtusb_hcd_request *request;
   struct virtusb_hcd_request *candidate;
   struct virtusb_device *device;
   unsigned long flags;

   if ((virt_hcd == NULL) || (hcd == NULL) || (owner == NULL)) {
      return;
   }

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);
   binding = virtusb_backend_find_owner_locked(&virt_hcd->backend, owner);
   if (binding != NULL) {
      device = binding->device;
      list_del_init(&binding->node);
   } else {
      device = NULL;
   }
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

   if (device == NULL) {
      return;
   }

   for (;;) {
      request = NULL;

      spin_lock_irqsave(&virt_hcd->backend.lock, flags);
      list_for_each_entry(candidate, &virt_hcd->backend.pending, node) {
         if (candidate->transfer->device == device) {
            request = candidate;
            break;
         }
      }

      if (request == NULL) {
         list_for_each_entry(candidate, &virt_hcd->backend.active, node) {
            if ((candidate->transfer->device == device) &&
                (candidate->owner == owner)) {
               request = candidate;
               break;
            }
         }
      }

      if (request != NULL) {
         list_del_init(&request->node);
         usb_hcd_unlink_urb_from_ep(hcd, request->urb);
      }
      spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

      if (request == NULL) {
         break;
      }

      (void)virtusb_transfer_cancel(request->transfer, -ESHUTDOWN);
      virtusb_backend_giveback(hcd, request, -ESHUTDOWN);
   }

   virtusb_object_put(&device->object);
   kfree(binding);
   wake_up_interruptible_all(&virt_hcd->backend.wait_queue);
}

void virtusb_backend_stop(struct virtusb_hcd *virt_hcd, struct usb_hcd *hcd)
{
   struct virtusb_backend_binding *binding;
   struct virtusb_hcd_request *request;
   unsigned long flags;

   if ((virt_hcd == NULL) || (hcd == NULL)) {
      return;
   }

   spin_lock_irqsave(&virt_hcd->backend.lock, flags);
   virt_hcd->backend.stopping = true;
   spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);
   wake_up_interruptible_all(&virt_hcd->backend.wait_queue);

   for (;;) {
      spin_lock_irqsave(&virt_hcd->backend.lock, flags);
      if (!list_empty(&virt_hcd->backend.pending)) {
         request = list_first_entry(&virt_hcd->backend.pending,
                                    struct virtusb_hcd_request,
                                    node);
         list_del_init(&request->node);
      } else if (!list_empty(&virt_hcd->backend.active)) {
         request = list_first_entry(&virt_hcd->backend.active,
                                    struct virtusb_hcd_request,
                                    node);
         list_del_init(&request->node);
      } else {
         request = NULL;
      }

      if (request != NULL) {
         usb_hcd_unlink_urb_from_ep(hcd, request->urb);
      }
      spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

      if (request == NULL) {
         break;
      }

      (void)virtusb_transfer_cancel(request->transfer, -ESHUTDOWN);
      virtusb_backend_giveback(hcd, request, -ESHUTDOWN);
   }

   for (;;) {
      spin_lock_irqsave(&virt_hcd->backend.lock, flags);
      if (!list_empty(&virt_hcd->backend.bindings)) {
         binding = list_first_entry(&virt_hcd->backend.bindings,
                                    struct virtusb_backend_binding,
                                    node);
         list_del_init(&binding->node);
      } else {
         binding = NULL;
      }
      spin_unlock_irqrestore(&virt_hcd->backend.lock, flags);

      if (binding == NULL) {
         break;
      }

      virtusb_object_put(&binding->device->object);
      kfree(binding);
   }
}
