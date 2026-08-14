// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/kref.h>
#include <linux/types.h>

#include <virtusb_uapi.h>

struct virtusb_object;

/**
 * typedef virtusb_object_release_t - Final-release callback
 * @object: Object whose last reference was released.
 *
 * The callback releases the complete concrete Core Component containing
 * @object. It is called exactly once when the final kref is dropped.
 */
typedef void (*virtusb_object_release_t)(struct virtusb_object *object);

/**
 * struct virtusb_object - Common kernel representation of a VirtUSB object
 * @id: Global runtime object ID, or VIRTUSB_OBJECT_ID_INVALID before registration.
 * @type: Concrete VirtUSB Core Component type.
 * @refcount: Reference-counted object lifetime.
 * @release: Concrete final-release callback.
 * @registered: Object is currently published by VirtUsbObjMgr.
 *
 * This structure is kernel-internal and is never exposed through the UAPI.
 *
 * Concrete Core Components embed this structure as their first member. This
 * allows the generic object layer to allocate storage for the complete
 * concrete object while remaining independent of the concrete type.
 */
struct virtusb_object {
   virtusb_object_id_t id;
   enum virtusb_object_type type;
   struct kref refcount;
   virtusb_object_release_t release;
   bool registered;
};

/**
 * virtusb_object_alloc() - Allocate generic storage for a concrete VirtUSB object
 * @size: Size of the complete concrete object.
 * @release: Concrete final-release callback.
 *
 * The returned allocation is zero-initialized and begins with an initialized
 * struct virtusb_object. The concrete type must embed struct virtusb_object as
 * its first member.
 *
 * Return: Pointer to the initialized common object or NULL on allocation
 * failure.
 */
struct virtusb_object *virtusb_object_alloc(size_t size,
                                            virtusb_object_release_t release);

/**
 * virtusb_object_get() - Acquire an additional object reference
 * @object: Object to reference.
 *
 * Return: @object.
 */
struct virtusb_object *virtusb_object_get(struct virtusb_object *object);

/**
 * virtusb_object_put() - Release an object reference
 * @object: Object reference to release.
 *
 * Passing NULL has no effect.
 */
void virtusb_object_put(struct virtusb_object *object);
