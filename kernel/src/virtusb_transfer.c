// SPDX-License-Identifier: GPL-2.0-only

#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "virtusb_device.h"
#include "virtusb_object.h"
#include "virtusb_transfer.h"

static atomic64_t virtusb_transfer_next_id = ATOMIC64_INIT(0);

static void virtusb_transfer_release(struct kref *refcount)
{
   struct virtusb_transfer *transfer;

   transfer = container_of(refcount, struct virtusb_transfer, refcount);

   kfree(transfer->buffer);
   virtusb_object_put(&transfer->device->object);
   kfree(transfer);
}

struct virtusb_transfer *virtusb_transfer_create(
   struct virtusb_device *device,
   enum virtusb_transfer_type type,
   enum virtusb_transfer_direction direction,
   u8 endpoint,
   const struct usb_ctrlrequest *setup,
   const void *data,
   size_t length,
   gfp_t gfp_flags)
{
   struct virtusb_transfer *transfer;

   if (device == NULL) {
      return NULL;
   }

   if ((type < VIRTUSB_TRANSFER_TYPE_CONTROL) ||
       (type > VIRTUSB_TRANSFER_TYPE_INTERRUPT)) {
      return NULL;
   }

   if ((direction != VIRTUSB_TRANSFER_DIRECTION_OUT) &&
       (direction != VIRTUSB_TRANSFER_DIRECTION_IN)) {
      return NULL;
   }

   if (endpoint > 15U) {
      return NULL;
   }

   if ((type == VIRTUSB_TRANSFER_TYPE_CONTROL) && (setup == NULL)) {
      return NULL;
   }

   if ((direction == VIRTUSB_TRANSFER_DIRECTION_OUT) && (length > 0U) &&
       (data == NULL)) {
      return NULL;
   }

   transfer = kzalloc(sizeof(*transfer), gfp_flags);
   if (transfer == NULL) {
      return NULL;
   }

   if (length > 0U) {
      transfer->buffer = kmalloc(length, gfp_flags);
      if (transfer->buffer == NULL) {
         kfree(transfer);
         return NULL;
      }

      if (direction == VIRTUSB_TRANSFER_DIRECTION_OUT) {
         memcpy(transfer->buffer, data, length);
      }
   }

   kref_init(&transfer->refcount);
   transfer->id = (u64)atomic64_inc_return(&virtusb_transfer_next_id);
   transfer->device = container_of(virtusb_object_get(&device->object),
                                   struct virtusb_device,
                                   object);
   transfer->type = type;
   transfer->direction = direction;
   transfer->state = VIRTUSB_TRANSFER_STATE_NEW;
   transfer->endpoint = endpoint;
   transfer->requested_length = length;
   transfer->status = -EINPROGRESS;

   if (setup != NULL) {
      transfer->has_setup = true;
      memcpy(&transfer->setup, setup, sizeof(transfer->setup));
   }

   return transfer;
}

struct virtusb_transfer *virtusb_transfer_get(struct virtusb_transfer *transfer)
{
   if (transfer != NULL) {
      kref_get(&transfer->refcount);
   }

   return transfer;
}

void virtusb_transfer_put(struct virtusb_transfer *transfer)
{
   if (transfer != NULL) {
      kref_put(&transfer->refcount, virtusb_transfer_release);
   }
}

int virtusb_transfer_mark_queued(struct virtusb_transfer *transfer)
{
   if (transfer == NULL) {
      return -EINVAL;
   }

   if (transfer->state != VIRTUSB_TRANSFER_STATE_NEW) {
      return -EINVAL;
   }

   transfer->state = VIRTUSB_TRANSFER_STATE_QUEUED;

   return 0;
}

int virtusb_transfer_complete(struct virtusb_transfer *transfer,
                              int status,
                              size_t actual_length)
{
   if (transfer == NULL) {
      return -EINVAL;
   }

   if ((transfer->state != VIRTUSB_TRANSFER_STATE_NEW) &&
       (transfer->state != VIRTUSB_TRANSFER_STATE_QUEUED)) {
      return -EINVAL;
   }

   if (actual_length > transfer->requested_length) {
      return -EMSGSIZE;
   }

   transfer->actual_length = actual_length;
   transfer->status = status;
   transfer->state = VIRTUSB_TRANSFER_STATE_COMPLETED;

   return 0;
}

int virtusb_transfer_cancel(struct virtusb_transfer *transfer, int status)
{
   if (transfer == NULL) {
      return -EINVAL;
   }

   if ((transfer->state != VIRTUSB_TRANSFER_STATE_NEW) &&
       (transfer->state != VIRTUSB_TRANSFER_STATE_QUEUED)) {
      return -EINVAL;
   }

   transfer->actual_length = 0U;
   transfer->status = status;
   transfer->state = VIRTUSB_TRANSFER_STATE_CANCELLED;

   return 0;
}
