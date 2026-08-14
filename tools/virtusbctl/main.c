// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <virtusb.h>

static void virtusbctl_print_usage(const char *program)
{
   fprintf(stderr,
           "Usage:\n"
           "  %s [-d INSTANCE] status [PORT]\n"
           "  %s [-d INSTANCE] device create SPEED[,SPEED...]\n"
           "  %s [-d INSTANCE] device destroy OBJECT_ID [--force]\n"
           "  %s [-d INSTANCE] device attach OBJECT_ID PORT\n"
           "  %s [-d INSTANCE] device detach OBJECT_ID\n"
           "  %s [-d INSTANCE] device connect OBJECT_ID\n"
           "  %s [-d INSTANCE] device disconnect OBJECT_ID\n"
           "  %s [-d INSTANCE] port power PORT on|off\n",
           program,
           program,
           program,
           program,
           program,
           program,
           program,
           program);
}

static const char *virtusbctl_speed_name(enum virtusb_speed speed)
{
   switch (speed) {
   case VIRTUSB_SPEED_NONE:
      return "none";

   case VIRTUSB_SPEED_LOW:
      return "low";

   case VIRTUSB_SPEED_FULL:
      return "full";

   case VIRTUSB_SPEED_HIGH:
      return "high";
   }

   return "unknown";
}

static void virtusbctl_print_status_flags(uint32_t status)
{
   bool first = true;

#define PRINT_STATUS_FLAG(_flag, _name) \
   do { \
      if ((status & (_flag)) != 0U) { \
         printf("%s%s", first ? "" : ",", (_name)); \
         first = false; \
      } \
   } while (0)

   PRINT_STATUS_FLAG(VIRTUSB_STATUS_CONNECTED, "connected");
   PRINT_STATUS_FLAG(VIRTUSB_STATUS_ENABLED, "enabled");
   PRINT_STATUS_FLAG(VIRTUSB_STATUS_SUSPENDED, "suspended");
   PRINT_STATUS_FLAG(VIRTUSB_STATUS_OVER_CURRENT, "over-current");
   PRINT_STATUS_FLAG(VIRTUSB_STATUS_RESET, "reset");
   PRINT_STATUS_FLAG(VIRTUSB_STATUS_POWER, "power");

#undef PRINT_STATUS_FLAG

   if (first) {
      printf("none");
   }
}

static void virtusbctl_print_change_flags(uint32_t change)
{
   bool first = true;

#define PRINT_CHANGE_FLAG(_flag, _name) \
   do { \
      if ((change & (_flag)) != 0U) { \
         printf("%s%s", first ? "" : ",", (_name)); \
         first = false; \
      } \
   } while (0)

   PRINT_CHANGE_FLAG(VIRTUSB_CHANGE_CONNECTED, "connected");
   PRINT_CHANGE_FLAG(VIRTUSB_CHANGE_ENABLED, "enabled");
   PRINT_CHANGE_FLAG(VIRTUSB_CHANGE_SUSPENDED, "suspended");
   PRINT_CHANGE_FLAG(VIRTUSB_CHANGE_OVER_CURRENT, "over-current");
   PRINT_CHANGE_FLAG(VIRTUSB_CHANGE_RESET, "reset");

#undef PRINT_CHANGE_FLAG

   if (first) {
      printf("none");
   }
}

static void virtusbctl_print_state(const char *label,
                                   const struct virtusb_state *state)
{
   printf("%-8s status=0x%08x [", label, state->status);
   virtusbctl_print_status_flags(state->status);

   printf("] change=0x%08x [", state->change);
   virtusbctl_print_change_flags(state->change);

   printf("] speed=%s\n", virtusbctl_speed_name(state->speed));
}

static int virtusbctl_parse_unsigned(const char *text, unsigned int *value)
{
   char *end;
   unsigned long parsed;

   if ((text == NULL) || (value == NULL) || (*text == '\0')) {
      return -EINVAL;
   }

   errno = 0;
   parsed = strtoul(text, &end, 0);

   if ((errno != 0) || (*end != '\0') || (parsed > UINT32_MAX)) {
      return -EINVAL;
   }

   *value = (unsigned int)parsed;

   return 0;
}

static int virtusbctl_parse_speed_caps(const char *text, uint32_t *speed_caps)
{
   char buffer[64];
   char *token;
   char *saveptr = NULL;
   uint32_t caps = 0U;
   size_t length;

   if ((text == NULL) || (speed_caps == NULL)) {
      return -EINVAL;
   }

   length = strlen(text);
   if ((length == 0U) || (length >= sizeof(buffer))) {
      return -EINVAL;
   }

   memcpy(buffer, text, length + 1U);

   for (token = strtok_r(buffer, ",", &saveptr);
        token != NULL;
        token = strtok_r(NULL, ",", &saveptr)) {
      if (strcmp(token, "low") == 0) {
         caps |= VIRTUSB_DEVICE_SPEED_LOW;
      } else if (strcmp(token, "full") == 0) {
         caps |= VIRTUSB_DEVICE_SPEED_FULL;
      } else if (strcmp(token, "high") == 0) {
         caps |= VIRTUSB_DEVICE_SPEED_HIGH;
      } else {
         return -EINVAL;
      }
   }

   if (caps == 0U) {
      return -EINVAL;
   }

   *speed_caps = caps;

   return 0;
}

static int virtusbctl_device_create(unsigned int instance, uint32_t speed_caps)
{
   struct virtusb_handle *handle;
   virtusb_object_id_t object_id;
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_device_create(handle, speed_caps, &object_id);
   if (ret < 0) {
      fprintf(stderr, "Failed to create device: %s\n", strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   printf("Device created: %u\n", object_id);

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

static int virtusbctl_device_destroy(unsigned int instance,
                                     virtusb_object_id_t object_id,
                                     bool force)
{
   struct virtusb_handle *handle;
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_device_destroy(handle, object_id, force);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to destroy device %u: %s\n",
              object_id,
              strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   printf("Device destroyed: %u%s\n",
          object_id,
          force ? " (forced)" : "");

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

static int virtusbctl_device_attach(unsigned int instance,
                                     virtusb_object_id_t object_id,
                                     unsigned int port)
{
   struct virtusb_handle *handle;
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_device_attach(handle, object_id, 0U, port);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to attach device %u to port %u: %s\n",
              object_id,
              port,
              strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   printf("Device %u attached to port %u\n", object_id, port);

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

static int virtusbctl_device_detach(unsigned int instance,
                                     virtusb_object_id_t object_id)
{
   struct virtusb_handle *handle;
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_device_detach(handle, object_id);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to detach device %u: %s\n",
              object_id,
              strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   printf("Device detached: %u\n", object_id);

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

static int virtusbctl_device_connection(unsigned int instance,
                                         virtusb_object_id_t object_id,
                                         bool connected)
{
   struct virtusb_handle *handle;
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_device_set_connected(handle, object_id, connected);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to %s device %u: %s\n",
              connected ? "connect" : "disconnect",
              object_id,
              strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   printf("Device %s: %u\n",
          connected ? "connected" : "disconnected",
          object_id);

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

static int virtusbctl_port_power(unsigned int instance,
                                 unsigned int port,
                                 bool powered)
{
   struct virtusb_handle *handle;
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_set_port_power(handle, 0U, port, powered);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to set port %u power %s: %s\n",
              port,
              powered ? "on" : "off",
              strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   printf("Port %u power: %s\n", port, powered ? "on" : "off");

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

static int virtusbctl_status(unsigned int instance, unsigned int port)
{
   struct virtusb_handle *handle;
   struct virtusb_status status;
   const char *device_path;
   unsigned int i;
   char label[32];
   int ret;

   ret = virtusb_open(instance, &handle);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to open /dev/virtusb%u: %s\n",
              instance,
              strerror(-ret));
      return EXIT_FAILURE;
   }

   ret = virtusb_get_port_status(handle, 0U, port, &status);
   if (ret < 0) {
      fprintf(stderr,
              "Failed to query port %u: %s\n",
              port,
              strerror(-ret));
      virtusb_close(handle);
      return EXIT_FAILURE;
   }

   device_path = virtusb_get_device_path(handle);

   printf("Device: %s\n", device_path != NULL ? device_path : "<unknown>");
   printf("Hub:    %u\n", status.hub_id);

   if (port == 0U) {
      printf("Ports:  %u\n\n", status.port_count);

      virtusbctl_print_state("hub:", &status.state);

      for (i = 0U; i < status.port_count; ++i) {
         snprintf(label, sizeof(label), "port %u:", i + 1U);
         virtusbctl_print_state(label, &status.ports[i]);
      }
   } else {
      printf("Port:   %u\n\n", status.port);

      snprintf(label, sizeof(label), "port %u:", status.port);
      virtusbctl_print_state(label, &status.state);
   }

   virtusb_close(handle);

   return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
   unsigned int instance = 0U;
   unsigned int value;
   unsigned int port = 0U;
   uint32_t speed_caps;
   bool force = false;
   int argument = 1;
   int ret;

   if ((argument < argc) && (strcmp(argv[argument], "-d") == 0)) {
      ++argument;

      if (argument >= argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      ret = virtusbctl_parse_unsigned(argv[argument], &instance);
      if (ret < 0) {
         fprintf(stderr, "Invalid instance: %s\n", argv[argument]);
         return EXIT_FAILURE;
      }

      ++argument;
   }

   if (argument >= argc) {
      virtusbctl_print_usage(argv[0]);
      return EXIT_FAILURE;
   }

   if (strcmp(argv[argument], "status") == 0) {
      ++argument;

      if (argument < argc) {
         ret = virtusbctl_parse_unsigned(argv[argument], &port);
         if ((ret < 0) || (port > VIRTUSB_MAX_PORTS)) {
            fprintf(stderr, "Invalid port: %s\n", argv[argument]);
            return EXIT_FAILURE;
         }

         ++argument;
      }

      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_status(instance, port);
   }

   if (strcmp(argv[argument], "port") == 0) {
      ++argument;

      if ((argument >= argc) || (strcmp(argv[argument], "power") != 0)) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_unsigned(argv[argument], &port) < 0) ||
          (port == 0U) ||
          (port > VIRTUSB_MAX_PORTS)) {
         fprintf(stderr, "Invalid port\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if (argument >= argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      if (strcmp(argv[argument], "on") == 0) {
         force = true;
      } else if (strcmp(argv[argument], "off") == 0) {
         force = false;
      } else {
         fprintf(stderr, "Power state must be 'on' or 'off'\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_port_power(instance, port, force);
   }

   if (strcmp(argv[argument], "device") != 0) {
      virtusbctl_print_usage(argv[0]);
      return EXIT_FAILURE;
   }

   ++argument;
   if (argument >= argc) {
      virtusbctl_print_usage(argv[0]);
      return EXIT_FAILURE;
   }

   if (strcmp(argv[argument], "create") == 0) {
      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_speed_caps(argv[argument], &speed_caps) < 0)) {
         fprintf(stderr, "Invalid device speed capability list\n");
         return EXIT_FAILURE;
      }

      ++argument;
      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_device_create(instance, speed_caps);
   }

   if (strcmp(argv[argument], "destroy") == 0) {
      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_unsigned(argv[argument], &value) < 0) ||
          (value == VIRTUSB_INVALID_OBJECT_ID)) {
         fprintf(stderr, "Invalid object ID\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if ((argument < argc) && (strcmp(argv[argument], "--force") == 0)) {
         force = true;
         ++argument;
      }

      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_device_destroy(instance,
                                       (virtusb_object_id_t)value,
                                       force);
   }

   if (strcmp(argv[argument], "attach") == 0) {
      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_unsigned(argv[argument], &value) < 0) ||
          (value == VIRTUSB_INVALID_OBJECT_ID)) {
         fprintf(stderr, "Invalid object ID\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_unsigned(argv[argument], &port) < 0) ||
          (port == 0U) ||
          (port > VIRTUSB_MAX_PORTS)) {
         fprintf(stderr, "Invalid port\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_device_attach(instance,
                                      (virtusb_object_id_t)value,
                                      port);
   }

   if (strcmp(argv[argument], "detach") == 0) {
      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_unsigned(argv[argument], &value) < 0) ||
          (value == VIRTUSB_INVALID_OBJECT_ID)) {
         fprintf(stderr, "Invalid object ID\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_device_detach(instance,
                                      (virtusb_object_id_t)value);
   }

   if ((strcmp(argv[argument], "connect") == 0) ||
       (strcmp(argv[argument], "disconnect") == 0)) {
      bool connected = strcmp(argv[argument], "connect") == 0;

      ++argument;

      if ((argument >= argc) ||
          (virtusbctl_parse_unsigned(argv[argument], &value) < 0) ||
          (value == VIRTUSB_INVALID_OBJECT_ID)) {
         fprintf(stderr, "Invalid object ID\n");
         return EXIT_FAILURE;
      }

      ++argument;

      if (argument != argc) {
         virtusbctl_print_usage(argv[0]);
         return EXIT_FAILURE;
      }

      return virtusbctl_device_connection(instance,
                                          (virtusb_object_id_t)value,
                                          connected);
   }

   virtusbctl_print_usage(argv[0]);

   return EXIT_FAILURE;
}
