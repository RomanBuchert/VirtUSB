// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <linux/types.h>

#include "virtusb_object.h"

/**
 * DOC: VirtUSB object manager
 *
 * VirtUsbObjMgr provides the common global registry for VirtUSB Core
 * Components. It manages object identity, publication, lookup, and the
 * registry reference held for every published object.
 *
 * It does not manage topology, USB protocol behavior, or type-specific
 * resources.
 */

/**
 * virtusb_object_manager_init() - Initialize the global object manager
 *
 * Must be called once before any VirtUSB object is registered.
 */
void virtusb_object_manager_init(void);

/**
 * virtusb_object_manager_exit() - Shut down the global object manager
 *
 * All registered objects must have been unregistered before this function is
 * called.
 */
void virtusb_object_manager_exit(void);

/**
 * virtusb_object_register() - Publish an initialized VirtUSB object
 * @object: Fully initialized but unpublished object.
 * @type: Concrete Core Component type.
 *
 * Assigns a monotonically allocated global ID, acquires the registry
 * reference, and publishes the object for lookup.
 *
 * Object IDs are never reused during one loaded module lifetime.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int virtusb_object_register(struct virtusb_object *object,
                            enum virtusb_object_type type);

/**
 * virtusb_object_unregister() - Remove an object from the global registry
 * @object: Published object to unpublish.
 *
 * Prevents new lookups and drops the object-manager registry reference.
 * Existing references remain valid until released by their holders.
 *
 * Return: 0 on success or a negative error code if @object is not currently
 * registered.
 */
int virtusb_object_unregister(struct virtusb_object *object);

/**
 * virtusb_object_lookup() - Look up an object by global object ID
 * @id: Global runtime object ID.
 *
 * A successful lookup acquires a reference. The caller must release it with
 * virtusb_object_put().
 *
 * Return: Referenced object on success or NULL if no published object has
 * @id.
 */
struct virtusb_object *virtusb_object_lookup(virtusb_object_id_t id);

/**
 * virtusb_object_lookup_first_by_type() - Look up the first object of a type
 * @type: Core Component type to find.
 *
 * This helper is intended for ordered module shutdown. A successful lookup
 * acquires a reference that must be released with virtusb_object_put().
 *
 * Return: Referenced object on success or NULL if no published object of
 * @type exists.
 */
struct virtusb_object *
virtusb_object_lookup_first_by_type(enum virtusb_object_type type);
