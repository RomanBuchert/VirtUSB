// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

struct file;
struct usb_hcd;
struct urb;
struct virtusb_device;
struct virtusb_hcd;
struct virtusb_transfer_fetch;
struct virtusb_transfer_complete_request;

struct virtusb_hcd_request {
   struct list_head node;
   struct urb *urb;
   struct virtusb_transfer *transfer;
   struct file *owner;
};

struct virtusb_backend_binding {
   struct list_head node;
   struct virtusb_device *device;
   struct file *owner;
};

struct virtusb_backend {
   spinlock_t lock;
   struct list_head bindings;
   struct list_head pending;
   struct list_head active;
   wait_queue_head_t wait_queue;
   bool stopping;
};

void virtusb_backend_init(struct virtusb_backend *backend);
void virtusb_backend_stop(struct virtusb_hcd *virt_hcd, struct usb_hcd *hcd);

int virtusb_backend_register(struct virtusb_hcd *virt_hcd,
                             struct file *owner,
                             struct virtusb_device *device);
void virtusb_backend_release_owner(struct virtusb_hcd *virt_hcd,
                                   struct usb_hcd *hcd,
                                   struct file *owner);
int virtusb_backend_submit(struct virtusb_hcd *virt_hcd,
                           struct usb_hcd *hcd,
                           struct urb *urb,
                           struct virtusb_transfer *transfer,
                           gfp_t mem_flags);
int virtusb_backend_cancel(struct virtusb_hcd *virt_hcd,
                           struct usb_hcd *hcd,
                           struct urb *urb,
                           int status);

int virtusb_backend_fetch(struct virtusb_hcd *virt_hcd,
                          struct file *owner,
                          struct virtusb_transfer_fetch *fetch,
                          bool nonblock);
int virtusb_backend_fetch_rollback(struct virtusb_hcd *virt_hcd,
                                   struct file *owner,
                                   u64 transfer_id);
int virtusb_backend_complete(struct virtusb_hcd *virt_hcd,
                             struct usb_hcd *hcd,
                             struct file *owner,
                             const struct virtusb_transfer_complete_request *completion);

bool virtusb_backend_has_pending(struct virtusb_hcd *virt_hcd, struct file *owner);
