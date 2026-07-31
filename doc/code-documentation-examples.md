# VirtUSB Code Documentation Examples

Status: Draft

# 1. Purpose

This document provides examples for documenting VirtUSB source code.

It supplements the coding and documentation constraints defined by the project
requirements.

# 2. General Rules

- Document public declarations in the corresponding header file.
- Do not duplicate documentation at the implementation.
- Document source-local functions where the additional information is useful.
- Simple local helper functions do not require formal API documentation.
- Keep documentation concise and focused on externally relevant behaviour.
- Do not restate information that is already evident from the declaration.

# 3. Userspace Documentation with Doxygen

## 3.1 Header File Example

```c
/**
 * @file virtusb_controller.h
 * @brief VirtUSB controller interface.
 *
 * This header declares the public userspace interface for managing a VirtUSB
 * controller connection.
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque VirtUSB controller handle.
 */
typedef struct virtusb_controller virtusb_controller_t;

/**
 * @brief Opens a VirtUSB controller.
 *
 * @param[in] path
 * Path to the controller device, for example `/dev/virtusb0`.
 *
 * @param[out] controller
 * Receives the opened controller handle on success.
 *
 * @return
 * Zero on success or a negative error code on failure.
 *
 * @pre `path` must not be `NULL`.
 * @pre `controller` must not be `NULL`.
 *
 * @post On success, `*controller` refers to an open controller.
 * @post On failure, `*controller` is set to `NULL`.
 */
int virtusb_controller_open(const char *path, virtusb_controller_t **controller);

/**
 * @brief Closes a VirtUSB controller.
 *
 * Any resources owned by the controller handle are released.
 *
 * @param[in] controller
 * Controller handle to close. A `NULL` value is permitted and has no effect.
 */
void virtusb_controller_close(virtusb_controller_t *controller);

#ifdef __cplusplus
}
#endif
```

## 3.2 Source File Example

```c
#include "virtusb_controller.h"

int virtusb_controller_open(const char *path, virtusb_controller_t **controller)
{
   /* Implementation without duplicated Doxygen documentation. */
}

void virtusb_controller_close(virtusb_controller_t *controller)
{
   /* Implementation without duplicated Doxygen documentation. */
}
```

## 3.3 Source-Local Function Example

```c
/**
 * @brief Validates a controller device path.
 *
 * @param[in] path
 * Path to validate.
 *
 * @return
 * `true` if the path is valid; otherwise `false`.
 */
static bool controller_path_is_valid(const char *path)
{
   /* Implementation */
}
```

A simple helper whose behaviour is obvious from its name and implementation does
not require formal Doxygen documentation.

# 4. Kernel Documentation with kernel-doc

## 4.1 Kernel Function Example

```c
/**
 * virtusb_controller_create() - Create a virtual USB controller
 * @index: Controller instance index
 *
 * Allocates and initializes one virtual USB controller instance.
 *
 * Return: Pointer to the controller on success or an ERR_PTR() value on failure.
 */
struct virtusb_controller *virtusb_controller_create(unsigned int index);
```

## 4.2 Kernel Structure Example

```c
/**
 * struct virtusb_controller - Virtual USB controller instance
 * @index: Controller instance index
 * @hcd: Associated USB Host Controller Driver instance
 * @ports: Root Hub port state
 *
 * Represents one independently operating VirtUSB controller.
 */
struct virtusb_controller {
   unsigned int index;
   struct usb_hcd *hcd;
   struct virtusb_port ports[VIRTUSB_PORT_COUNT];
};
```

# 5. Documentation to Avoid

Avoid documentation that merely repeats the declaration:

```c
/**
 * @brief Gets the state.
 *
 * @return The state.
 */
int virtusb_get_state(void);
```

Instead, document information that is not evident from the declaration, such as
ownership, valid states, error behaviour, concurrency constraints, and lifetime
requirements.
