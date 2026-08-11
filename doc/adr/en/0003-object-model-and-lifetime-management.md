# ADR-0003: Object Model and Lifetime Management

- **Status:** Proposed
- **Date:** 2026-08-10

---

## Context

VirtUSB defines the following Core Components:

- `VirtUsbHcd`
- `VirtUsbRHub`
- `VirtUsbHub`
- `VirtUsbDev`

Core Components represent independently identifiable runtime entities of
the VirtUSB system.

The High-Level Architecture establishes that the existence of a Core
Component is independent of its integration into a VirtUSB topology.

In particular, detaching a `VirtUsbDev` or `VirtUsbHub` from a downstream
port does not destroy the corresponding component. Attachment,
USB-visible state, and object existence therefore represent
different concepts and must not implicitly control each other's
lifetime.

Downstream ports are not independent Core Components. They are integral
parts of their respective `VirtUsbRHub` or `VirtUsbHub` and consequently
do not require an independent object identity or lifetime.

As the Control Plane evolves, Core Components must be individually
identifiable from kernel space and user space. Object lookup and
destruction may also occur concurrently with operations that temporarily
reference an object.

A common object model is therefore required that defines:

- which runtime entities are VirtUSB objects,
- how objects are identified,
- how object identifiers are allocated,
- how objects are registered and discovered,
- how object lifetime is protected,
- how concrete Core Components are constructed,
- and which parts of the object model are visible through the UAPI.

The kernel-internal object representation must remain separate from the
public `libvirtusb` API. Future language wrappers, including C++ and
Python wrappers, operate exclusively on `libvirtusb` and must not depend
on kernel-internal structures.

---

## Decision

### VirtUSB Objects

Every VirtUSB Core Component is a VirtUSB object:

```text
VirtUsbHcd
VirtUsbRHub
VirtUsbHub
VirtUsbDev
```

A VirtUSB object has its own identity and managed lifetime.

Object existence is independent of Attachment or membership in a VirtUSB
topology.

Consequently:

- detaching an object does not destroy it,
- removing an object from a topology does not destroy it,
- USB-visible state transitions do not determine object lifetime,
- and object destruction is an explicit lifecycle operation.

Entities that do not have independent existence are not VirtUSB objects.

In particular, downstream ports are integral parts of their respective
hub object and are identified through the hub and their port number.


Although USB ports are not Core Components and are not managed by
`VirtUsbObjMgr`, they are represented internally by the dedicated
`VirtUsbPort` runtime type.

`VirtUsbPort` is a component-owned subobject. Its lifetime is completely bound
to its owning Core Component. It therefore has no global object ID, no
independent `kref`, and no independent registration in the object manager.

The same `VirtUsbPort` type is used for upstream and downstream ports. A port
role identifies which USB semantics apply. `VirtUsbDev` owns one upstream
port, `VirtUsbHub` owns one upstream port plus its downstream ports, and
`VirtUsbRHub` owns downstream ports only.

A port stores only its own local capabilities, properties, and state.
Properties whose semantics are common to both roles belong to the common port
representation; role-specific properties belong to the corresponding upstream
or downstream representation.

Attachment is represented by a reciprocal peer relationship between one
downstream port and one upstream port. There is no separate runtime state for
Association versus Attachment, and no independent device-to-HCD Association
is stored.

Effective USB-visible properties are determined from the virtual hardware
state and the applicable USB protocol rules. They are not maintained as a
redundant persistent copy inside `VirtUsbPort`. For example, each port stores
only its supported-speed capability mask; the current USB operating speed is
determined by the USB protocol layer and is not stored in `VirtUsbPort`.

Packed bitmaps and other aggregate representations are also not maintained as
a second copy of port state. They are generated from `VirtUsbPort` instances
only when required for UAPI transfer, USB hub-class translation, or another
explicit serialization boundary. Incoming aggregate representations are
decoded directly into the corresponding port state.

This single-source-of-truth model prevents divergence between local port
state, effective connection state, and serialized bitmap representations.


Attachment is represented directly by reciprocal `peer` references between one
downstream and one upstream `VirtUsbPort`. VirtUSB deliberately does not keep a
separate Association state or an additional `attached` flag:

```text
peer == NULL     -> detached
peer != NULL     -> attached
```

The term Association may still describe the resulting structural relationship,
but there is no independent `associated && !attached` runtime state.

`VirtUsbPort` contains only virtual-hardware and topology state. USB-defined
protocol state such as suspend, connection, enable, reset, `PORT_*`, and
`C_PORT_*` does not belong to the port hardware object.

The local `speed` field is a capability bit mask. The current USB operating
speed is determined by the USB protocol layer and is not stored in
`VirtUsbPort`.

Downstream-port hardware contains the actual VBUS/power state and, where the
hub uses per-port protection, the local over-current condition.

### Common Kernel Object Representation

The kernel uses a common internal base representation for all VirtUSB
objects.

Conceptually:

```c
struct virtusb_object {
   virtusb_object_id_t id;
   enum virtusb_object_type type;
   struct kref refcount;

   /* Internal object-management state. */
};
```

Each concrete Core Component embeds this common object representation.

Conceptually:

```c
struct virtusb_device {
   struct virtusb_object object;

   /* Device-specific state. */
};
```

The common object representation is an internal kernel implementation
detail and is not part of the VirtUSB UAPI.

### Object Manager

VirtUSB provides a common object manager, referred to as
`VirtUsbObjMgr`.

The object manager is responsible for generic object-management
functions including:

- object ID allocation,
- object registration,
- object unregistration,
- object publication,
- lookup by object ID,
- and management of the registry reference held for published objects.

The object manager does not implement the functional behavior of
concrete Core Components.

In particular, it does not manage:

- topology relationships,
- attachment,
- USB protocol state,
- hub or port state machines,
- device behavior,
- or USB transfers.

The object manager does not require knowledge of the concrete
implementation of `VirtUsbHcd`, `VirtUsbRHub`, `VirtUsbHub`, or
`VirtUsbDev`.

It therefore does not use type-specific construction logic or a central
switch over concrete Core Component implementations.

### Object Allocation and Construction

Object construction is divided into distinct phases:

```text
Allocate
   ↓
Unregistered object storage
   ↓
Concrete component initialization
   ↓
Registration
   ↓
Published VirtUSB object
```

The common object layer allocates and initializes the type-independent
object storage required for the complete concrete object.

The concrete Core Component then initializes its type-specific portion
and resources.

Only after successful concrete initialization is the object registered
with `VirtUsbObjMgr`.

Registration:

- assigns the concrete object type,
- assigns the globally unique object ID,
- inserts the object into the object registry,
- and makes the object available for lookup.

An incompletely initialized object is never published through the object
manager.

This keeps allocation and common lifetime mechanics generic while
leaving type-specific initialization under the control of the concrete
Core Component implementation.

### Global Object ID Namespace

All VirtUSB objects share one global object ID namespace.

The object ID representation is 32 bits:

```c
typedef __u32 virtusb_object_id_t;
```

Object ID `0` is reserved as the invalid object ID.

Valid IDs therefore occupy the range:

```text
1 .. UINT32_MAX
```

An object ID identifies exactly one VirtUSB object during one loaded
lifetime of the VirtUSB kernel module.

Object IDs are independent of object type. No bits of the object ID are
reserved for or interpreted as type information.

Type and identity are separate properties.

### Object ID Allocation

Object IDs are allocated monotonically.

An object ID is never reused during one loaded lifetime of the VirtUSB
kernel module, even after the corresponding object has been destroyed.

For example:

```text
ID 42 -> VirtUsbDev A
destroy VirtUsbDev A
ID 42 -> remains unused
```

ID 42 must not subsequently identify another object.

This prevents stale object references from accidentally resolving to a
different object created later.

Object ID allocation must not wrap around.

If the 32-bit object ID namespace is exhausted, creation or registration
of additional objects fails.

If practical use later demonstrates that a 32-bit namespace is
insufficient, the object ID representation should be enlarged rather
than introducing ID reuse.

When the VirtUSB kernel module is unloaded and loaded again, a new
object-ID allocation lifetime begins and IDs may again start at `1`.

Object IDs are therefore runtime identifiers and are not persistent
system-wide identifiers.

### Object Types

The object type is represented separately from the object ID.

The object type definitions are shared through the VirtUSB UAPI so that
kernel space, user space, and `libvirtusb` use the same numeric type
values.

Conceptually:

```c
enum virtusb_object_type {
   VIRTUSB_OBJECT_TYPE_INVALID = 0,
   VIRTUSB_OBJECT_TYPE_HCD,
   VIRTUSB_OBJECT_TYPE_ROOT_HUB,
   VIRTUSB_OBJECT_TYPE_HUB,
   VIRTUSB_OBJECT_TYPE_DEVICE,
};
```

Object type `0` is reserved as invalid or unspecified.

The concrete numeric representation becomes part of the UAPI once the
UAPI is stabilized.

The kernel must not maintain a separate, independently numbered object
type enumeration.

### Reference-Counted Lifetime

All VirtUSB objects use reference-counted lifetime management based on
the Linux kernel `kref` mechanism.

The object manager owns one reference for every registered object.

A successful object lookup acquires an additional reference before the
object is returned to the caller.

The caller must release this reference after it no longer accesses the
object.

Object destruction is logically separated from final memory release.

The destruction sequence is:

```text
Destroy requested
   ↓
Remove object from registry
   ↓
Prevent new lookups
   ↓
Drop object-manager reference
   ↓
Object remains alive while references exist
   ↓
Final reference released
   ↓
Release concrete object
```

Removing an object from the object manager therefore makes it
unavailable for new operations without invalidating references already
held by operations in progress.

The concrete Core Component is responsible for releasing its
type-specific resources when the final reference is dropped.

The generic object layer provides the common reference-counting
mechanism but does not contain type-specific destruction logic.

### Hub Hardware Capabilities

Hub hardware capabilities are modeled independently from USB protocol state.

VirtUSB supports two USB 2.0 power-switching modes:

- `GANGED`
- `INDIVIDUAL`

The historical USB 1.0 no-power-switching mode is not modeled.

Actual VBUS state remains stored per downstream `VirtUsbPort`. In ganged mode,
hardware operations change all downstream-port power states together rather
than introducing a redundant hub-wide power-state field.

VirtUSB supports three over-current protection modes:

- `GLOBAL`
- `PER_PORT`
- `NONE`

A global over-current condition is hub-local hardware state. A per-port
over-current condition is downstream-port-local hardware state.

The USB hub protocol layer converts these hardware capabilities and states into
the USB 2.0 descriptor, status, and change representations.

### Object Relationships

The object manager manages object identity and lifetime, not
relationships between objects.

Relationships such as:

```text
VirtUsbHcd <-> VirtUsbRHub
Downstream VirtUsbPort <-> Upstream VirtUsbPort (Attachment)
```

are represented and maintained by the corresponding functional
components.

Attachment is the local parent-child topology relationship between one
downstream `VirtUsbPort` and one upstream `VirtUsbPort`. Membership in a
particular `VirtUsbHcd` topology is derived by traversing these reciprocal
peer relationships to the `VirtUsbRHub`; it is not represented by an
independent device-to-HCD relationship.

Consequently, moving the root of an attached hub subtree to another VirtUSB
topology requires changing only the Attachment of the subtree root. The
Attachments within that subtree remain unchanged.

In particular, the following concepts remain distinct:

```text
Object existence
Attachment / topology relationship
USB-visible state
```

Changing an Attachment or USB-visible state does not implicitly destroy the
object.

### Kernel, UAPI, Library, and Wrapper Separation

The VirtUSB object model is separated into four interface levels:

```text
VirtUSB Kernel
   ↓
VirtUSB UAPI
   ↓
libvirtusb
   ↓
Language Wrappers
```

The kernel contains the private object implementation, including
`struct virtusb_object`, `kref`, registry state, and other
kernel-specific data.

The UAPI exposes only ABI-stable representations required for
communication between kernel space and user space, including object IDs,
object types, and explicit request or response structures.

Kernel-internal structures and their memory layout are never exposed
through the UAPI.

`libvirtusb` provides its own public C API on top of the UAPI.

The public library API does not expose or depend on the layout of
kernel-internal object structures.

Future C++ and Python wrappers are built exclusively on top of
`libvirtusb`.

They do not access:

- kernel-internal VirtUSB structures,
- kernel implementation details,
- or the VirtUSB UAPI directly.

This preserves the freedom to evolve the kernel implementation and UAPI
independently of the language-specific wrapper interfaces.

---

## Considered Alternatives

### Core Component Lifetime Owned by Topology Relationships

A Core Component is created as part of a topology and destroyed when it
is detached or otherwise removed from that topology.

#### Assessment

Rejected.

This conflicts with the established architecture rule that Core
Component existence is independent of topology integration.

It would also unnecessarily couple object lifetime to attachment and USB
state.

### HCD-Owned Device Lifetime

A `VirtUsbDev` is owned by the `VirtUsbHcd` whose topology currently contains it.

#### Assessment

Rejected.

Topology membership is a relationship between independently existing objects
and must not implicitly determine device lifetime.

A device must be able to exist while detached and outside any HCD topology.

### Per-Type Object ID Namespaces

Each object type maintains an independent ID namespace.

For example, HCD 1, Hub 1, and Device 1 could exist simultaneously.

#### Assessment

Rejected.

An object ID would not uniquely identify an object without additionally
specifying its type.

A common global namespace provides simpler lookup semantics and cleaner
Control-Plane interfaces.

### Encoding Object Type in the Object ID

Part of the 32-bit object ID is reserved for the object type.

#### Assessment

Rejected.

Identity and type are separate properties.

Encoding type information in the ID would reduce the available ID space,
couple identifier layout to the current set of object types, and provide
no required functional benefit.

### Reusing Object IDs

Object IDs of destroyed objects are returned to the available ID pool.

#### Assessment

Rejected.

Reusing IDs would allow stale references to accidentally identify a
newly created and unrelated object.

The 32-bit namespace is considered sufficient for the intended use. If
this assumption later proves incorrect, the identifier representation
should be enlarged.

### Object Manager Constructs Concrete Components

`VirtUsbObjMgr` contains type-specific construction logic and creates
`VirtUsbHcd`, `VirtUsbRHub`, `VirtUsbHub`, and `VirtUsbDev` directly.

#### Assessment

Rejected.

The Core Components have different initialization requirements and
dependencies.

Making the object manager aware of these details would tightly couple it
to all concrete component implementations and turn it into a central
factory and lifecycle controller.

### Concrete Components Allocate Themselves Independently

Each Core Component performs its own complete memory allocation and
initializes the common object representation before registering itself
with `VirtUsbObjMgr`.

#### Assessment

Technically acceptable, but not selected.

This keeps the object manager independent of concrete types, but
duplicates common allocation and object-initialization responsibilities
across component implementations.

The selected model instead centralizes type-independent object
allocation while retaining type-specific initialization in each
component.

### Parallel Per-Port and Hub-Bitmap State

Port state is stored both in individual `VirtUsbPort` instances and in
persistent hub-wide bitmaps.

#### Assessment

Rejected.

Maintaining both representations as authoritative state would require every
state transition to update two independent data structures atomically. Any
missed or partial update could leave the port object and the bitmap
representation inconsistent.

The selected model keeps `VirtUsbPort` as the single source of truth and uses
bitmaps only as transient aggregate or transfer representations.

### Object Manager Without Reference Counting

Registered objects are stored in a registry and are destroyed
immediately when removed.

#### Assessment

Rejected.

Concurrent Control-Plane operations or other kernel users may still hold
a reference to an object when destruction begins.

Reference-counted lifetime management provides a defined separation
between object unpublication and final memory release.

### Separate Kernel and UAPI Object Type Enumerations

Kernel space and user space maintain independent object type
enumerations.

#### Assessment

Rejected.

The types represent the same semantic entities on both sides of the
interface.

A shared UAPI definition prevents unnecessary translation and avoids
diverging numeric representations.

---

## Consequences

### Advantages

- Core Component lifetime is independent of topology relationships.
- Every VirtUSB object has a globally unambiguous runtime identity.
- Stale object IDs cannot accidentally resolve to newly created objects
  during the same module lifetime.
- The object manager remains independent of concrete Core Component
  implementations.
- Incompletely initialized objects cannot become visible through the
  registry.
- `kref` provides defined lifetime behavior for concurrent object access
  and destruction.
- Kernel-internal object structures remain private.
- The UAPI has a uniform object identity and type model.
- `libvirtusb` can provide a coherent public object-oriented C API
  without exposing kernel implementation details.
- C++ and Python wrappers can be developed exclusively against
  `libvirtusb`.
- Future Core Component types can use the same generic object
  infrastructure.
- Upstream and downstream ports share the same `VirtUsbPort` base model.
- Each port stores only its own local capabilities, properties, and state.
- Effective connection properties are derived from both peer ports according
  to USB rules rather than stored redundantly.
- Port state has one canonical internal representation in `VirtUsbPort`.
- Bitmap and packed representations cannot silently diverge from persistent
  per-port state because they are generated only at explicit boundaries.

### Disadvantages

- The object model introduces additional infrastructure before all Core
  Components require dynamic lifecycle management.
- Every managed object requires reference-counting and registry
  bookkeeping.
- Object destruction becomes asynchronous with respect to final memory
  release when references remain outstanding.
- Object ID exhaustion must be handled explicitly.
- The distinction between allocation, concrete initialization,
  registration, unregistration, and final release requires disciplined
  lifecycle handling.
- Aggregate bitmap generation and decoding require explicit conversion code
  at interface boundaries.

---

## Implementation

The decision is expected to introduce or affect at least the following
components:

```text
uapi/include/virtusb_uapi.h

kernel/include/virtusb_object.h
kernel/include/virtusb_object_manager.h

kernel/src/virtusb_object.c
kernel/src/virtusb_object_manager.c
```

The existing Core Components will progressively embed the common object
representation:

```text
VirtUsbHcd
VirtUsbRHub
VirtUsbHub
VirtUsbDev
```

The object model should be introduced incrementally. Existing functional
behavior should remain unchanged while Core Components are migrated to
the common object infrastructure.
