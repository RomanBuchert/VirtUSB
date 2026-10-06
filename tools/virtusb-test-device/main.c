// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <virtusb.h>

#define USB_DIR_IN              0x80U
#define USB_TYPE_MASK           0x60U
#define USB_TYPE_STANDARD       0x00U
#define USB_RECIP_MASK          0x1fU
#define USB_RECIP_DEVICE        0x00U
#define USB_REQ_GET_STATUS      0x00U
#define USB_REQ_SET_ADDRESS     0x05U
#define USB_REQ_GET_DESCRIPTOR  0x06U
#define USB_REQ_GET_CONFIGURATION 0x08U
#define USB_REQ_SET_CONFIGURATION 0x09U
#define USB_DT_DEVICE           0x01U
#define USB_DT_CONFIG           0x02U

static volatile sig_atomic_t stop_requested;

static const uint8_t device_descriptor[] = {
   18U, USB_DT_DEVICE,
   0x00U, 0x02U,
   0x00U, 0x00U, 0x00U,
   64U,
   0xadU, 0xdeU,
   0x01U, 0x00U,
   0x00U, 0x01U,
   0U, 0U, 0U,
   1U,
};

static const uint8_t configuration_descriptor[] = {
   9U, USB_DT_CONFIG,
   18U, 0U,
   1U,
   1U,
   0U,
   0x80U,
   50U,

   9U, 0x04U,
   0U,
   0U,
   0U,
   0xffU,
   0x00U,
   0x00U,
   0U,
};

static uint16_t get_le16(const uint8_t *data)
{
   return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}


static void print_usage(FILE *stream, const char *program)
{
   fprintf(stream, "Usage: %s [INSTANCE [PORT]]\n", program);
}

static int parse_unsigned(const char *text, unsigned int *value)
{
   char *end = NULL;
   unsigned long parsed;

   if ((text == NULL) || (value == NULL) || (*text == '\0')) {
      return -EINVAL;
   }

   errno = 0;
   parsed = strtoul(text, &end, 0);
   if ((errno == ERANGE) || (parsed > UINT_MAX)) {
      return -ERANGE;
   }

   if ((end == text) || (*end != '\0')) {
      return -EINVAL;
   }

   *value = (unsigned int)parsed;
   return 0;
}

static void handle_signal(int signal_number)
{
   (void)signal_number;
   stop_requested = 1;
}

static int complete_data(struct virtusb_handle *handle,
                         const struct virtusb_transfer_request *request,
                         const void *data,
                         size_t length)
{
   size_t actual_length;

   actual_length = length;
   if (actual_length > request->requested_length) {
      actual_length = request->requested_length;
   }

   return virtusb_transfer_complete(handle,
                                    request->id,
                                    0,
                                    data,
                                    (uint32_t)actual_length);
}

static int handle_control_transfer(struct virtusb_handle *handle,
                                   const struct virtusb_transfer_request *request)
{
   uint8_t response[2] = {0U, 0U};
   uint8_t request_type;
   uint8_t usb_request;
   uint16_t value;
   uint8_t descriptor_type;

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
   fflush(stdout);

   if ((request_type & USB_TYPE_MASK) != USB_TYPE_STANDARD) {
      return virtusb_transfer_complete(handle, request->id, -EPIPE, NULL, 0U);
   }

   switch (usb_request) {
   case USB_REQ_GET_DESCRIPTOR:
      if ((request_type & USB_DIR_IN) == 0U) {
         break;
      }

      descriptor_type = (uint8_t)(value >> 8);
      if (descriptor_type == USB_DT_DEVICE) {
         return complete_data(handle,
                              request,
                              device_descriptor,
                              sizeof(device_descriptor));
      }

      if (descriptor_type == USB_DT_CONFIG) {
         return complete_data(handle,
                              request,
                              configuration_descriptor,
                              sizeof(configuration_descriptor));
      }
      break;

   case USB_REQ_SET_ADDRESS:
   case USB_REQ_SET_CONFIGURATION:
      if ((request_type & USB_DIR_IN) == 0U) {
         return virtusb_transfer_complete(handle, request->id, 0, NULL, 0U);
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

int main(int argc, char **argv)
{
   struct virtusb_transfer_request request;
   struct virtusb_handle *handle = NULL;
   virtusb_object_id_t object_id = VIRTUSB_INVALID_OBJECT_ID;
   unsigned int instance = 0U;
   unsigned int port = 1U;
   int ret;
   int exit_code = EXIT_FAILURE;

   if ((argc == 2) &&
       ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
      print_usage(stdout, argv[0]);
      return EXIT_SUCCESS;
   }

   if (argc > 3) {
      print_usage(stderr, argv[0]);
      return EXIT_FAILURE;
   }

   if (argc >= 2) {
      ret = parse_unsigned(argv[1], &instance);
      if (ret < 0) {
         fprintf(stderr, "Invalid INSTANCE: %s\n", argv[1]);
         print_usage(stderr, argv[0]);
         return EXIT_FAILURE;
      }
   }

   if (argc >= 3) {
      ret = parse_unsigned(argv[2], &port);
      if ((ret < 0) || (port == 0U)) {
         fprintf(stderr, "Invalid PORT: %s\n", argv[2]);
         print_usage(stderr, argv[0]);
         return EXIT_FAILURE;
      }
   }

   {
      struct sigaction action;

      memset(&action, 0, sizeof(action));
      action.sa_handler = handle_signal;
      sigemptyset(&action.sa_mask);

      if ((sigaction(SIGINT, &action, NULL) != 0) ||
          (sigaction(SIGTERM, &action, NULL) != 0)) {
         fprintf(stderr, "sigaction: %s\n", strerror(errno));
         return EXIT_FAILURE;
      }
   }

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr, "virtusb_open: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_device_create(handle, VIRTUSB_DEVICE_SPEED_FULL, &object_id);
   if (ret < 0) {
      fprintf(stderr, "device create: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_device_attach(handle, object_id, 0U, port);
   if (ret < 0) {
      fprintf(stderr, "device attach: %s\n", strerror(-ret));
      goto out;
   }

   printf("VirtUSB test device %u attached to port %u\n", object_id, port);
   fflush(stdout);

   ret = virtusb_backend_register(handle, object_id);
   if (ret < 0) {
      fprintf(stderr, "backend register: %s\n", strerror(-ret));
      goto out;
   }

   ret = virtusb_device_set_connected(handle, object_id, true);
   if (ret < 0) {
      fprintf(stderr, "device connect: %s\n", strerror(-ret));
      goto out;
   }

   while (!stop_requested) {
      ret = virtusb_transfer_fetch(handle, &request);
      if (ret == -EINTR) {
         continue;
      }
      if (ret < 0) {
         fprintf(stderr, "transfer fetch: %s\n", strerror(-ret));
         goto out;
      }

      if (request.object_id != object_id) {
         ret = virtusb_transfer_complete(handle, request.id, -ENODEV, NULL, 0U);
      } else if ((request.type == VIRTUSB_TRANSFER_CONTROL) &&
                 (request.endpoint == 0U)) {
         ret = handle_control_transfer(handle, &request);
      } else {
         ret = virtusb_transfer_complete(handle, request.id, -EPIPE, NULL, 0U);
      }

      if (ret < 0) {
         fprintf(stderr, "transfer complete: %s\n", strerror(-ret));
         goto out;
      }
   }

   exit_code = EXIT_SUCCESS;

out:
   if ((handle != NULL) && (object_id != VIRTUSB_INVALID_OBJECT_ID)) {
      (void)virtusb_device_set_connected(handle, object_id, false);
      (void)virtusb_device_detach(handle, object_id);
      (void)virtusb_backend_unregister(handle);
      (void)virtusb_device_destroy(handle, object_id, true);
   }

   virtusb_close(handle);
   return exit_code;
}
