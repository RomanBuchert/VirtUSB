// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/kref.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>

struct virtusb_device;

/**
 * DOC: VirtUSB transfer model
 *
 * VirtUsbTransfer is the transport-independent representation of one USB
 * transfer inside the VirtUSB core. It deliberately contains no Linux
 * struct urb and no userspace ABI representation.
 *
 * A transfer owns a reference to its target VirtUsbDev and owns its payload
 * buffer. This makes transfer lifetime independent of the originating host
 * controller request and provides a stable boundary for a later backend/data
 * plane implementation.
 */

enum virtusb_transfer_type {
   VIRTUSB_TRANSFER_TYPE_CONTROL = 0,
   VIRTUSB_TRANSFER_TYPE_ISOCHRONOUS,
   VIRTUSB_TRANSFER_TYPE_BULK,
   VIRTUSB_TRANSFER_TYPE_INTERRUPT,
};

enum virtusb_transfer_direction {
   VIRTUSB_TRANSFER_DIRECTION_OUT = 0,
   VIRTUSB_TRANSFER_DIRECTION_IN,
};

enum virtusb_transfer_state {
   VIRTUSB_TRANSFER_STATE_NEW = 0,
   VIRTUSB_TRANSFER_STATE_QUEUED,
   VIRTUSB_TRANSFER_STATE_COMPLETED,
   VIRTUSB_TRANSFER_STATE_CANCELLED,
};

struct virtusb_transfer {
   struct kref refcount;
   struct virtusb_device *device;

   enum virtusb_transfer_type type;
   enum virtusb_transfer_direction direction;
   enum virtusb_transfer_state state;

   u8 endpoint;
   bool has_setup;
   struct usb_ctrlrequest setup;

   void *buffer;
   size_t requested_length;
   size_t actual_length;
   int status;
};

struct virtusb_transfer *virtusb_transfer_create(
   struct virtusb_device *device,
   enum virtusb_transfer_type type,
   enum virtusb_transfer_direction direction,
   u8 endpoint,
   const struct usb_ctrlrequest *setup,
   const void *data,
   size_t length,
   gfp_t gfp_flags);

struct virtusb_transfer *virtusb_transfer_get(struct virtusb_transfer *transfer);
void virtusb_transfer_put(struct virtusb_transfer *transfer);

int virtusb_transfer_mark_queued(struct virtusb_transfer *transfer);
int virtusb_transfer_complete(struct virtusb_transfer *transfer,
                              int status,
                              size_t actual_length);
int virtusb_transfer_cancel(struct virtusb_transfer *transfer, int status);
