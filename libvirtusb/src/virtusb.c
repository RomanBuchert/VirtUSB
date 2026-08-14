// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <virtusb_uapi.h>

#include "virtusb.h"

#define VIRTUSB_DEVICE_PATH_MAX 64U

struct virtusb_handle {
   int fd;
   unsigned int instance;
   char device_path[VIRTUSB_DEVICE_PATH_MAX];
};

static enum virtusb_speed virtusb_speed_from_uapi(__u32 speed)
{
   switch (speed) {
   case VIRTUSB_PORT_SPEED_LOW:
      return VIRTUSB_SPEED_LOW;

   case VIRTUSB_PORT_SPEED_FULL:
      return VIRTUSB_SPEED_FULL;

   case VIRTUSB_PORT_SPEED_HIGH:
      return VIRTUSB_SPEED_HIGH;

   case VIRTUSB_PORT_SPEED_NONE:
   default:
      return VIRTUSB_SPEED_NONE;
   }
}

static void virtusb_state_from_uapi(struct virtusb_state *destination,
                                    const struct virtusb_port_state *source)
{
   destination->status = source->status;
   destination->change = source->change;
   destination->speed = virtusb_speed_from_uapi(source->speed);
}

int virtusb_open(unsigned int instance, struct virtusb_handle **handle)
{
   struct virtusb_handle *new_handle;
   char path[VIRTUSB_DEVICE_PATH_MAX];
   int length;
   int fd;

   if (handle == NULL) {
      return -EINVAL;
   }

   *handle = NULL;

   length = snprintf(path, sizeof(path), "/dev/virtusb%u", instance);
   if ((length < 0) || ((size_t)length >= sizeof(path))) {
      return -ENAMETOOLONG;
   }

   fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      return -errno;
   }

   new_handle = calloc(1U, sizeof(*new_handle));
   if (new_handle == NULL) {
      int saved_errno = errno;

      close(fd);

      if (saved_errno == 0) {
         saved_errno = ENOMEM;
      }

      return -saved_errno;
   }

   new_handle->fd = fd;
   new_handle->instance = instance;

   length = snprintf(new_handle->device_path,
                     sizeof(new_handle->device_path),
                     "%s",
                     path);
   if ((length < 0) || ((size_t)length >= sizeof(new_handle->device_path))) {
      close(fd);
      free(new_handle);

      return -ENAMETOOLONG;
   }

   *handle = new_handle;

   return 0;
}

void virtusb_close(struct virtusb_handle *handle)
{
   if (handle == NULL) {
      return;
   }

   if (handle->fd >= 0) {
      close(handle->fd);
   }

   free(handle);
}

const char *virtusb_get_device_path(const struct virtusb_handle *handle)
{
   if (handle == NULL) {
      return NULL;
   }

   return handle->device_path;
}

int virtusb_get_port_status(struct virtusb_handle *handle,
                            uint32_t hub_id,
                            uint32_t port,
                            struct virtusb_status *status)
{
   struct virtusb_port_status request;
   unsigned int i;
   int ret;

   if ((handle == NULL) || (status == NULL)) {
      return -EINVAL;
   }

   if (handle->fd < 0) {
      return -EBADF;
   }

   memset(&request, 0, sizeof(request));

   request.hub_id = (__u32)hub_id;
   request.port = (__u32)port;

   ret = ioctl(handle->fd, VIRTUSB_IOCTL_GET_PORT_STATUS, &request);
   if (ret < 0) {
      return -errno;
   }

   memset(status, 0, sizeof(*status));

   status->hub_id = request.hub_id;
   status->port = request.port;
   status->port_count = request.port_count;

   virtusb_state_from_uapi(&status->state, &request.state);

   if (request.port == 0U) {
      for (i = 0U;
           (i < request.port_count) && (i < VIRTUSB_MAX_PORTS);
           ++i) {
         virtusb_state_from_uapi(&status->ports[i], &request.ports[i]);
      }
   }

   return 0;
}

int virtusb_device_create(struct virtusb_handle *handle,
                          uint32_t speed_caps,
                          virtusb_object_id_t *object_id)
{
   struct virtusb_device_create request;
   int ret;

   if ((handle == NULL) || (object_id == NULL)) {
      return -EINVAL;
   }

   if (handle->fd < 0) {
      return -EBADF;
   }

   memset(&request, 0, sizeof(request));
   request.speed_caps = (__u32)speed_caps;

   ret = ioctl(handle->fd, VIRTUSB_IOCTL_DEVICE_CREATE, &request);
   if (ret < 0) {
      return -errno;
   }

   *object_id = (virtusb_object_id_t)request.object_id;

   return 0;
}

int virtusb_device_destroy(struct virtusb_handle *handle,
                           virtusb_object_id_t object_id,
                           bool force)
{
   struct virtusb_device_destroy request;
   int ret;

   if (handle == NULL) {
      return -EINVAL;
   }

   if (handle->fd < 0) {
      return -EBADF;
   }

   memset(&request, 0, sizeof(request));
   request.object_id = (__u32)object_id;
   request.flags = force ? VIRTUSB_DEVICE_DESTROY_FORCE : 0U;

   ret = ioctl(handle->fd, VIRTUSB_IOCTL_DEVICE_DESTROY, &request);
   if (ret < 0) {
      return -errno;
   }

   return 0;
}
