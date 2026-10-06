// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <virtusb.h>

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

static int complete_data(struct virtusb_handle *handle,
                         const struct virtusb_transfer_request *request,
                         const void *data,
                         size_t length)
{
   size_t actual_length = length;

   if (actual_length > request->requested_length) {
      actual_length = request->requested_length;
   }

   return virtusb_transfer_complete(handle, request->id, 0, data, (uint32_t)actual_length);
}

static int handle_control_transfer(struct virtusb_handle *handle,
                                   const struct virtusb_transfer_request *request,
                                   bool *configured)
{
   uint8_t response[2] = {0U, 0U};
   uint8_t request_type;
   uint8_t usb_request;
   uint16_t value;

   if (!request->has_setup) {
      return virtusb_transfer_complete(handle, request->id, -EINVAL, NULL, 0U);
   }

   request_type = request->setup[0];
   usb_request = request->setup[1];
   value = get_le16(&request->setup[2]);

   printf("transfer=%llu object=%u setup=%02x %02x %04x %04x %04x\n",
          (unsigned long long)request->id,
          request->object_id,
          request_type,
          usb_request,
          value,
          get_le16(&request->setup[4]),
          get_le16(&request->setup[6]));

   if ((request_type & USB_TYPE_MASK) != USB_TYPE_STANDARD) {
      return virtusb_transfer_complete(handle, request->id, -EPIPE, NULL, 0U);
   }

   switch (usb_request) {
   case USB_REQ_GET_DESCRIPTOR:
      if ((request_type & USB_DIR_IN) != 0U) {
         if ((uint8_t)(value >> 8) == USB_DT_DEVICE) {
            return complete_data(handle, request, device_descriptor, sizeof(device_descriptor));
         }
         if ((uint8_t)(value >> 8) == USB_DT_CONFIG) {
            return complete_data(handle,
                                 request,
                                 configuration_descriptor,
                                 sizeof(configuration_descriptor));
         }
      }
      break;

   case USB_REQ_SET_ADDRESS:
      if ((request_type & USB_DIR_IN) == 0U) {
         return virtusb_transfer_complete(handle, request->id, 0, NULL, 0U);
      }
      break;

   case USB_REQ_SET_CONFIGURATION:
      if ((request_type & USB_DIR_IN) == 0U) {
         int ret = virtusb_transfer_complete(handle, request->id, 0, NULL, 0U);
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
         return complete_data(handle, request, response, 1U);
      }
      break;

   case USB_REQ_GET_STATUS:
      if ((request_type & USB_DIR_IN) != 0U) {
         return complete_data(handle, request, response, sizeof(response));
      }
      break;

   default:
      break;
   }

   return virtusb_transfer_complete(handle, request->id, -EPIPE, NULL, 0U);
}

static int verify_port(struct virtusb_handle *handle, unsigned int port)
{
   struct virtusb_status status;
   uint32_t required;
   int ret;

   memset(&status, 0, sizeof(status));
   ret = virtusb_get_port_status(handle, 0U, port, &status);
   if (ret < 0) {
      return ret;
   }

   required = VIRTUSB_STATUS_CONNECTED | VIRTUSB_STATUS_ENABLED | VIRTUSB_STATUS_POWER;
   printf("port %u status=0x%08x speed=%u\n", port, status.state.status, status.state.speed);

   if ((status.state.status & required) != required) {
      return -EIO;
   }

   return 0;
}

int main(void)
{
   struct virtusb_transfer_request request;
   struct virtusb_handle *handle = NULL;
   virtusb_object_id_t object_id = VIRTUSB_INVALID_OBJECT_ID;
   bool configured = false;
   bool registered = false;
   int ret;
   int exit_code = EXIT_FAILURE;

   ret = virtusb_open(0U, &handle);
   if (ret < 0) {
      fprintf(stderr, "virtusb_open: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_device_create(handle, VIRTUSB_DEVICE_SPEED_FULL, &object_id);
   if (ret < 0) {
      fprintf(stderr, "device create: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_device_attach(handle, object_id, 0U, 1U);
   if (ret < 0) {
      fprintf(stderr, "device attach: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_backend_register(handle, object_id);
   if (ret < 0) {
      fprintf(stderr, "backend register: %s\n", strerror(-ret));
      goto out;
   }
   registered = true;

   ret = virtusb_device_set_connected(handle, object_id, true);
   if (ret < 0) {
      fprintf(stderr, "device connect: %s\n", strerror(-ret));
      goto out;
   }

   while (!configured) {
      ret = virtusb_transfer_fetch(handle, &request);
      if (ret < 0) {
         fprintf(stderr, "transfer fetch: %s\n", strerror(-ret));
         goto out;
      }

      if ((request.object_id != object_id) ||
          (request.type != VIRTUSB_TRANSFER_CONTROL) || (request.endpoint != 0U)) {
         ret = virtusb_transfer_complete(handle, request.id, -EPIPE, NULL, 0U);
      } else {
         ret = handle_control_transfer(handle, &request, &configured);
      }
      if (ret < 0) {
         fprintf(stderr, "transfer complete: %s\n", strerror(-ret));
         goto out;
      }
   }

   ret = verify_port(handle, 1U);
   if (ret < 0) {
      fprintf(stderr, "port state before unregister is invalid: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_backend_unregister(handle);
   if (ret < 0) {
      fprintf(stderr, "backend unregister: %s\n", strerror(-ret));
      goto out;
   }
   registered = false;

   ret = verify_port(handle, 1U);
   if (ret < 0) {
      fprintf(stderr, "port state changed by backend unregister: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_backend_register(handle, object_id);
   if (ret < 0) {
      fprintf(stderr, "backend re-register: %s\n", strerror(-ret));
      goto out;
   }
   registered = true;

   ret = verify_port(handle, 1U);
   if (ret < 0) {
      fprintf(stderr, "port state changed by backend re-register: %s\n", strerror(-ret));
      goto out;
   }

   puts("PASS: backend unregister/re-register did not change attachment or connection state");
   exit_code = EXIT_SUCCESS;

out:
   if (handle != NULL) {
      if (object_id != VIRTUSB_INVALID_OBJECT_ID) {
         (void)virtusb_device_set_connected(handle, object_id, false);
         (void)virtusb_device_detach(handle, object_id);
         if (registered) {
            (void)virtusb_backend_unregister(handle);
         }
         (void)virtusb_device_destroy(handle, object_id, true);
      }
      virtusb_close(handle);
   }

   return exit_code;
}
