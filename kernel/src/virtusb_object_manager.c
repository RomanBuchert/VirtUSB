// SPDX-License-Identifier: GPL-2.0-only

#include <linux/errno.h>
#include <linux/limits.h>
#include <linux/mutex.h>
#include <linux/xarray.h>

#include "virtusb_object_manager.h"

static DEFINE_XARRAY(virtusb_objects);
static DEFINE_MUTEX(virtusb_object_manager_lock);

static u64 virtusb_next_object_id;

void virtusb_object_manager_init(void)
{
   virtusb_next_object_id = 1U;
}

void virtusb_object_manager_exit(void)
{
   WARN_ON(!xa_empty(&virtusb_objects));
   xa_destroy(&virtusb_objects);
}

int virtusb_object_register(struct virtusb_object *object,
                            enum virtusb_object_type type)
{
   virtusb_object_id_t id;
   int ret;

   if (object == NULL) {
      return -EINVAL;
   }

   if ((type <= VIRTUSB_OBJECT_TYPE_INVALID) ||
       (type > VIRTUSB_OBJECT_TYPE_DEVICE)) {
      return -EINVAL;
   }

   mutex_lock(&virtusb_object_manager_lock);

   if (object->registered) {
      ret = -EALREADY;
      goto unlock;
   }

   if (virtusb_next_object_id > U32_MAX) {
      ret = -ENOSPC;
      goto unlock;
   }

   id = (virtusb_object_id_t)virtusb_next_object_id;

   /*
    * Acquire the registry reference before publication. The caller retains its
    * existing reference independently.
    */
   virtusb_object_get(object);

   ret = xa_insert(&virtusb_objects, (unsigned long)id, object, GFP_KERNEL);
   if (ret < 0) {
      virtusb_object_put(object);
      goto unlock;
   }

   object->id = id;
   object->type = type;
   object->registered = true;

   ++virtusb_next_object_id;

unlock:
   mutex_unlock(&virtusb_object_manager_lock);

   return ret;
}

int virtusb_object_unregister(struct virtusb_object *object)
{
   struct virtusb_object *registered;
   int ret = 0;

   if (object == NULL) {
      return -EINVAL;
   }

   mutex_lock(&virtusb_object_manager_lock);

   if ((!object->registered) ||
       (object->id == VIRTUSB_OBJECT_ID_INVALID)) {
      ret = -ENOENT;
      goto unlock;
   }

   registered = xa_load(&virtusb_objects, (unsigned long)object->id);
   if (registered != object) {
      ret = -ENOENT;
      goto unlock;
   }

   xa_erase(&virtusb_objects, (unsigned long)object->id);
   object->registered = false;

unlock:
   mutex_unlock(&virtusb_object_manager_lock);

   if (ret == 0) {
      virtusb_object_put(object);
   }

   return ret;
}

struct virtusb_object *virtusb_object_lookup(virtusb_object_id_t id)
{
   struct virtusb_object *object;

   if (id == VIRTUSB_OBJECT_ID_INVALID) {
      return NULL;
   }

   mutex_lock(&virtusb_object_manager_lock);

   object = xa_load(&virtusb_objects, (unsigned long)id);
   if (object != NULL) {
      virtusb_object_get(object);
   }

   mutex_unlock(&virtusb_object_manager_lock);

   return object;
}

struct virtusb_object *
virtusb_object_lookup_first_by_type(enum virtusb_object_type type)
{
   struct virtusb_object *object;
   unsigned long index;

   mutex_lock(&virtusb_object_manager_lock);

   xa_for_each(&virtusb_objects, index, object) {
      if (object->type == type) {
         virtusb_object_get(object);
         mutex_unlock(&virtusb_object_manager_lock);
         return object;
      }
   }

   mutex_unlock(&virtusb_object_manager_lock);

   return NULL;
}
