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
           "  %s [-d INSTANCE] status [PORT]\n",
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
   unsigned int port = 0U;
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

   if ((argument >= argc) || (strcmp(argv[argument], "status") != 0)) {
      virtusbctl_print_usage(argv[0]);
      return EXIT_FAILURE;
   }

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
