// SPDX-License-Identifier: GPL-2.0-only

#include <linux/slab.h>

#include "virtusb_object.h"

static void virtusb_object_release_kref(struct kref *refcount)
{
   struct virtusb_object *object;

   object = container_of(refcount, struct virtusb_object, refcount);
   object->release(object);
}

struct virtusb_object *virtusb_object_alloc(size_t size,
                                            virtusb_object_release_t release)
{
   struct virtusb_object *object;

   if ((size < sizeof(*object)) || (release == NULL)) {
      return NULL;
   }

   object = kzalloc(size, GFP_KERNEL);
   if (object == NULL) {
      return NULL;
   }

   object->id = VIRTUSB_OBJECT_ID_INVALID;
   object->type = VIRTUSB_OBJECT_TYPE_INVALID;
   object->release = release;
   object->registered = false;

   kref_init(&object->refcount);

   return object;
}

struct virtusb_object *virtusb_object_get(struct virtusb_object *object)
{
   if (object != NULL) {
      kref_get(&object->refcount);
   }

   return object;
}

void virtusb_object_put(struct virtusb_object *object)
{
   if (object != NULL) {
      kref_put(&object->refcount, virtusb_object_release_kref);
   }
}
