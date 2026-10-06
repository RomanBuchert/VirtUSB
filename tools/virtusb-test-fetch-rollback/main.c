// SPDX-License-Identifier: GPL-2.0-only

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <virtusb_uapi.h>

#define USB_DIR_IN                0x80U
#define USB_TYPE_MASK             0x60U
#define USB_TYPE_STANDARD         0x00U
#define USB_RECIP_MASK            0x1fU
#define USB_RECIP_DEVICE          0x00U
#define USB_REQ_GET_STATUS        0x00U
#define USB_REQ_SET_ADDRESS       0x05U
#define USB_REQ_GET_DESCRIPTOR    0x06U
#define USB_REQ_GET_CONFIGURATION 0x08U
#define USB_REQ_SET_CONFIGURATION 0x09U
#define USB_DT_DEVICE             0x01U
#define USB_DT_CONFIG             0x02U

static const uint8_t device_descriptor[] = {
   18U, USB_DT_DEVICE, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 64U, 0xadU,
   0xdeU, 0x01U, 0x00U, 0x00U, 0x01U, 0U, 0U, 0U, 1U,
};

static const uint8_t configuration_descriptor[] = {
   9U, USB_DT_CONFIG, 18U, 0U, 1U, 1U, 0U, 0x80U, 50U,
   9U, 0x04U, 0U, 0U, 0U, 0xffU, 0x00U, 0x00U, 0U,
};

static uint16_t get_le16(const uint8_t *data)
{
   return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int do_ioctl(int fd, unsigned long command, void *argument)
{
   if (ioctl(fd, command, argument) < 0) {
      return -errno;
   }
   return 0;
}

static int complete_transfer(int fd,
                             uint64_t transfer_id,
                             int status,
                             const void *data,
                             size_t length)
{
   struct virtusb_transfer_complete_request completion;

   if (length > VIRTUSB_TRANSFER_DATA_MAX) {
      return -EMSGSIZE;
   }

   memset(&completion, 0, sizeof(completion));
   completion.transfer_id = transfer_id;
   completion.status = status;
   completion.actual_length = (__u32)length;
   if ((data != NULL) && (length > 0U)) {
      memcpy(completion.data, data, length);
   }

   return do_ioctl(fd, VIRTUSB_IOCTL_TRANSFER_COMPLETE, &completion);
}

static int complete_data(int fd,
                         const struct virtusb_transfer_fetch *request,
                         const void *data,
                         size_t length)
{
   if (length > request->requested_length) {
      length = request->requested_length;
   }
   return complete_transfer(fd, request->transfer_id, 0, data, length);
}

static int handle_control_transfer(int fd,
                                   const struct virtusb_transfer_fetch *request,
                                   bool *configured)
{
   uint8_t response[2] = {0U, 0U};
   uint8_t request_type;
   uint8_t usb_request;
   uint16_t value;

   if (request->has_setup == 0U) {
      return complete_transfer(fd, request->transfer_id, -EINVAL, NULL, 0U);
   }

   request_type = request->setup[0];
   usb_request = request->setup[1];
   value = get_le16(&request->setup[2]);

   printf("transfer=%llu object=%u setup=%02x %02x %04x %04x %04x\n",
          (unsigned long long)request->transfer_id,
          request->object_id,
          request_type,
          usb_request,
          value,
          get_le16(&request->setup[4]),
          get_le16(&request->setup[6]));

   if ((request_type & USB_TYPE_MASK) != USB_TYPE_STANDARD) {
      return complete_transfer(fd, request->transfer_id, -EPIPE, NULL, 0U);
   }

   switch (usb_request) {
   case USB_REQ_GET_DESCRIPTOR:
      if ((request_type & USB_DIR_IN) != 0U) {
         if ((uint8_t)(value >> 8) == USB_DT_DEVICE) {
            return complete_data(fd, request, device_descriptor, sizeof(device_descriptor));
         }
         if ((uint8_t)(value >> 8) == USB_DT_CONFIG) {
            return complete_data(fd,
                                 request,
                                 configuration_descriptor,
                                 sizeof(configuration_descriptor));
         }
      }
      break;

   case USB_REQ_SET_ADDRESS:
      if ((request_type & USB_DIR_IN) == 0U) {
         return complete_transfer(fd, request->transfer_id, 0, NULL, 0U);
      }
      break;

   case USB_REQ_SET_CONFIGURATION:
      if ((request_type & USB_DIR_IN) == 0U) {
         int ret = complete_transfer(fd, request->transfer_id, 0, NULL, 0U);
         if (ret == 0) {
            *configured = true;
         }
         return ret;
      }
      break;

   case USB_REQ_GET_CONFIGURATION:
      if ((request_type & (USB_DIR_IN | USB_RECIP_MASK)) ==
          (USB_DIR_IN | USB_RECIP_DEVICE)) {
         response[0] = 1U;
         return complete_data(fd, request, response, 1U);
      }
      break;

   case USB_REQ_GET_STATUS:
      if ((request_type & USB_DIR_IN) != 0U) {
         return complete_data(fd, request, response, sizeof(response));
      }
      break;

   default:
      break;
   }

   return complete_transfer(fd, request->transfer_id, -EPIPE, NULL, 0U);
}

int main(void)
{
   struct virtusb_device_create create;
   struct virtusb_device_attach attach;
   struct virtusb_backend_register registration;
   struct virtusb_device_connection connection;
   struct virtusb_device_object object;
   struct virtusb_device_destroy destroy;
   struct virtusb_transfer_fetch fetch;
   bool created = false;
   bool attached = false;
   bool registered = false;
   bool connected = false;
   bool configured = false;
   int fd = -1;
   int ret;
   int exit_code = EXIT_FAILURE;

   fd = open("/dev/virtusb0", O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      fprintf(stderr, "open /dev/virtusb0: %s\n", strerror(errno));
      goto out;
   }

   memset(&create, 0, sizeof(create));
   create.speed_caps = VIRTUSB_SPEED_CAP_FULL;
   ret = do_ioctl(fd, VIRTUSB_IOCTL_DEVICE_CREATE, &create);
   if (ret < 0) {
      fprintf(stderr, "device create: %s\n", strerror(-ret));
      goto out;
   }
   created = true;

   memset(&attach, 0, sizeof(attach));
   attach.object_id = create.object_id;
   attach.hub_id = VIRTUSB_ROOT_HUB_ID;
   attach.port = 1U;
   ret = do_ioctl(fd, VIRTUSB_IOCTL_DEVICE_ATTACH, &attach);
   if (ret < 0) {
      fprintf(stderr, "device attach: %s\n", strerror(-ret));
      goto out;
   }
   attached = true;

   memset(&registration, 0, sizeof(registration));
   registration.object_id = create.object_id;
   ret = do_ioctl(fd, VIRTUSB_IOCTL_BACKEND_REGISTER, &registration);
   if (ret < 0) {
      fprintf(stderr, "backend register: %s\n", strerror(-ret));
      goto out;
   }
   registered = true;

   memset(&connection, 0, sizeof(connection));
   connection.object_id = create.object_id;
   connection.enabled = 1U;
   ret = do_ioctl(fd, VIRTUSB_IOCTL_DEVICE_CONNECTION, &connection);
   if (ret < 0) {
      fprintf(stderr, "device connect: %s\n", strerror(-ret));
      goto out;
   }
   connected = true;

   errno = 0;
   if (ioctl(fd, VIRTUSB_IOCTL_TRANSFER_FETCH, (void *)(uintptr_t)1U) >= 0) {
      fputs("faulting fetch unexpectedly succeeded\n", stderr);
      goto out;
   }
   if (errno != EFAULT) {
      fprintf(stderr, "faulting fetch returned %s instead of EFAULT\n", strerror(errno));
      goto out;
   }
   puts("faulting fetch returned EFAULT as expected");

   memset(&fetch, 0, sizeof(fetch));
   ret = do_ioctl(fd, VIRTUSB_IOCTL_TRANSFER_FETCH, &fetch);
   if (ret < 0) {
      fprintf(stderr, "fetch after EFAULT: %s\n", strerror(-ret));
      goto out;
   }
   if ((fetch.transfer_id == 0U) || (fetch.object_id != create.object_id)) {
      fputs("fetch after EFAULT returned an invalid transfer\n", stderr);
      goto out;
   }
   printf("rollback recovered transfer=%llu object=%u\n",
          (unsigned long long)fetch.transfer_id,
          fetch.object_id);

   for (;;) {
      if ((fetch.type != VIRTUSB_UAPI_TRANSFER_TYPE_CONTROL) || (fetch.endpoint != 0U)) {
         ret = complete_transfer(fd, fetch.transfer_id, -EPIPE, NULL, 0U);
      } else {
         ret = handle_control_transfer(fd, &fetch, &configured);
      }
      if (ret < 0) {
         fprintf(stderr, "transfer complete: %s\n", strerror(-ret));
         goto out;
      }
      if (configured) {
         break;
      }

      memset(&fetch, 0, sizeof(fetch));
      ret = do_ioctl(fd, VIRTUSB_IOCTL_TRANSFER_FETCH, &fetch);
      if (ret < 0) {
         fprintf(stderr, "transfer fetch: %s\n", strerror(-ret));
         goto out;
      }
   }

   puts("PASS: EFAULT rolled the transfer back without unregistering the backend");
   exit_code = EXIT_SUCCESS;

out:
   if (fd >= 0) {
      if (connected) {
         memset(&connection, 0, sizeof(connection));
         connection.object_id = create.object_id;
         connection.enabled = 0U;
         (void)do_ioctl(fd, VIRTUSB_IOCTL_DEVICE_CONNECTION, &connection);
      }
      if (attached) {
         memset(&object, 0, sizeof(object));
         object.object_id = create.object_id;
         (void)do_ioctl(fd, VIRTUSB_IOCTL_DEVICE_DETACH, &object);
      }
      if (registered) {
         (void)do_ioctl(fd, VIRTUSB_IOCTL_BACKEND_UNREGISTER, NULL);
      }
      if (created) {
         memset(&destroy, 0, sizeof(destroy));
         destroy.object_id = create.object_id;
         destroy.flags = VIRTUSB_DEVICE_DESTROY_FORCE;
         (void)do_ioctl(fd, VIRTUSB_IOCTL_DEVICE_DESTROY, &destroy);
      }
      close(fd);
   }

   return exit_code;
}
