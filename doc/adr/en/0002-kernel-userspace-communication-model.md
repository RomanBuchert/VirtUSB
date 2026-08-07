# ADR-0002: Kernel--Userspace Communication Model

-   **Status:** Proposed
-   **Date:** 2026-08-07

------------------------------------------------------------------------

## Context

VirtUSB requires bidirectional communication between kernel space and
user space.

Two semantically distinct communication paths are required:

-   **Control Plane**
-   **USB Data Plane**

The Control Plane controls states and operations of the virtual hardware
that are not themselves part of USB protocol communication. This
includes creation and destruction of virtual objects, attach and detach
operations, device and port power, USB connect and disconnect, topology
changes, and other virtual hardware state changes outside USB protocol
traffic.

The USB Data Plane transports USB-side operations between the Linux USB
core and a user-space backend. This includes Control, Bulk, Interrupt,
and Isochronous transfers, USB reset, suspend and resume, transfer
cancellation and completion, and other events caused by USB protocol
operation.

Linux-internal data structures such as `struct urb` must not become part
of the VirtUSB kernel--user-space ABI.

VirtUSB also requires an efficient mechanism for transferring
potentially large or frequent USB payloads without requiring the
complete payload to be copied between kernel space and user space for
every transfer.

------------------------------------------------------------------------

## Decision

Each `VirtUsbHcd` is represented by its own character-device endpoint:

``` text
/dev/virtusb0
/dev/virtusb1
...
```

An open file descriptor represents a connection to exactly one
`VirtUsbHcd` and its associated VirtUSB topology.

The Control Plane and USB Data Plane remain semantically separate, but
both use the same HCD-specific character-device endpoint.

``` mermaid
flowchart TB
    APP["Application / Device Simulator"]
    LIB["libvirtusb"]
    DEV["/dev/virtusbX"]

    subgraph USER["User Space"]
        APP --> LIB
    end

    subgraph KERNEL["Kernel Space"]
        CTRL["Control Plane"]
        DATA["USB Data Plane"]
        HCD["VirtUsbHcd X"]
        TOPO["VirtUSB Topology"]

        CTRL --> HCD
        DATA --> HCD
        HCD --> TOPO
    end

    LIB --> DEV
    DEV --> CTRL
    DEV --> DATA
```

### Control Plane

Control-Plane operations are performed using `ioctl()`.

They carry small, explicit control operations and do not transport USB
payload data. The concrete set and binary representation of the
`ioctl()` operations are defined by a later ABI specification.

### Control-Plane State Access Model

The Control Plane provides two complementary views of hub and port state.

#### Port-Oriented View

The port-oriented view addresses one position within a hub and returns the
complete state associated with that position.

VirtUSB uses a common position numbering model:

- position 0 represents the hub itself,
- positions 1 through 31 represent downstream ports 1 through 31.

A port-oriented query for a downstream port therefore answers the question:

> What is the complete state of this port?

The returned state may include, as applicable:

- power,
- connection,
- enable,
- suspend,
- reset,
- over-current,
- USB speed,
- pending change states.

A query for position 0 addresses the hub itself. It may additionally return a
snapshot of the states of all downstream ports belonging to that hub. This
provides an efficient way for diagnostic tools, test programs, TUI
applications, or GUI applications to obtain the complete current hub state.

The exact binary representation of such a snapshot is defined by the later ABI
specification.

#### Status-Oriented View

The status-oriented view addresses one state property and returns that property
for the hub and all of its downstream ports.

It therefore answers questions such as:

> Which ports are currently powered?

or:

> Which ports currently report a connected device?

Boolean states are represented as 32-bit bitmaps using the common VirtUSB
position numbering model:

``` text
bit 0       Hub
bits 1..31  Downstream ports 1..31
```

This representation applies naturally to boolean properties such as power,
connection, enable, suspend, reset, and over-current state.

Multi-bit properties such as USB speed use an equivalently position-oriented
packed representation with enough bits per position to represent all valid
property values.

Status-oriented queries return the complete state representation. The kernel
does not apply a caller-provided port mask. A user-space consumer that is
interested only in selected ports applies the required mask to the returned
value itself.

This keeps the kernel ABI simple and deterministic and avoids unnecessary
filtering semantics for values that are already compact.

#### State Modification

The Control Plane may modify only states that represent externally controllable
or simulated virtual-hardware conditions.

This includes, in particular:

- port power,
- device connection and disconnection,
- over-current conditions,
- connection speed.

States that result from USB protocol operation or host-controller behavior are
not directly controlled through this interface merely because they are
observable through the status-oriented or port-oriented views.

This distinction preserves the separation between the Control Plane and the
USB Data Plane. For example, enable, suspend, and reset states may be observable
through the Control Plane while their transitions remain governed by USB
protocol and HCD operation.

The concrete set of readable and writable properties, `ioctl()` commands, and
binary ABI structures is defined by the later ABI specification.

### USB Data Plane

The USB Data Plane uses shared memory. The shared-memory region is
mapped into user space using `mmap()`.

At least two logically separate unidirectional communication rings are
provided within the shared-memory region:

-   Kernel → User Space
-   User Space → Kernel

Both directions use the same generic ring model. The producer and
consumer roles are reversed depending on the direction.

USB payload buffers are also located within the shared-memory region.
The exact number and size of rings, descriptor format, and organization
of payload storage are intentionally left to a later detailed design.

``` mermaid
flowchart LR
    K["Kernel"]

    subgraph SHM["Shared Memory"]
        KUR["Kernel → User Ring"]
        PAY["Payload Buffers"]
        UKR["User → Kernel Ring"]
    end

    U["User-Space Backend"]

    K -->|produce| KUR
    KUR -->|consume| U
    K <-->|USB payload| PAY
    U <-->|USB payload| PAY
    U -->|produce| UKR
    UKR -->|consume| K
```

### Kernel → User-Space Notification

After the kernel places new entries into the Kernel→User-Space ring, it
signals the character device through its `poll()` implementation.

User space can wait for this condition using `poll()` or `epoll()`.

### User-Space → Kernel Notification

Changing shared memory alone does not cause execution to enter the
kernel.

After user space has placed new entries into the User-Space→Kernel ring,
it notifies the kernel using an `ioctl()` operation.

This operation acts only as a **doorbell**. Transfer descriptors and
payload data remain in shared memory.

``` mermaid
sequenceDiagram
    participant K as Kernel
    participant SHM as Shared Memory
    participant U as User Space

    K->>SHM: Produce USB operation
    K-->>U: wake_up() / poll readiness
    U->>SHM: Consume operation
    U->>SHM: Produce completion / response
    U->>K: ioctl() doorbell
    K->>SHM: Consume completion / response
```

### Synchronization Model

Shared memory and notification are separate mechanisms.

-   Shared memory transports descriptors and USB payload data.
-   `poll()`/`epoll()` provides Kernel→User-Space notification.
-   `ioctl()` provides User-Space→Kernel notification.

Notifications should be batchable. A notification is not required for
every individual ring entry when multiple entries can be processed
together.

The ring abstraction itself remains symmetric and generic:

``` text
Producer
   ↓
write ring entry
   ↓
advance producer index
   ↓
notify peer
   ↓
Consumer
   ↓
consume available entries
   ↓
advance consumer index
```

The Linux-specific implementation of `notify peer` is intentionally
asymmetric:

``` text
Kernel → User Space: poll()/epoll() readiness
User Space → Kernel: ioctl() doorbell
```

------------------------------------------------------------------------

## Addressing Model

The character-device endpoint already identifies the associated
`VirtUsbHcd`.

Objects within that HCD's VirtUSB topology are addressed using HCD-local
identifiers.

A port is conceptually identified by:

``` text
Parent Hub + Port Number
```

The same addressing model applies to ports of the `VirtUsbRHub` and
ports of ordinary `VirtUsbHub` instances.

For example:

``` text
{ hub = ROOT_HUB, port = 3 }
{ hub = 17,       port = 2 }
```

The concrete binary representation and identifier types are defined by a
later ABI specification.

``` mermaid
flowchart TB
    D["/dev/virtusbX"]
    HCD["VirtUsbHcd X"]
    RH["VirtUsbRHub"]
    RP1["Port 1"]
    RP2["Port 2"]
    HUB["VirtUsbHub (ID 17)"]
    HP1["Port 1"]
    HP2["Port 2"]

    D --> HCD
    HCD --> RH
    RH --> RP1
    RH --> RP2
    RP2 --> HUB
    HUB --> HP1
    HUB --> HP2
```

------------------------------------------------------------------------

## Considered Alternatives

### configfs for the Control Plane

configfs could represent virtual objects, properties, and relationships
through a file-system hierarchy.

**Decision:** Rejected.

VirtUSB requires a programmatic user-space library and a separate
high-performance USB Data Plane regardless. Dynamic operations such as
attach, detach, power, connect, and disconnect are operation-oriented
and are expected to be triggered by running test programs, TUI
applications, or GUI applications.

Adding configfs would introduce a second user-space control interface
without a currently demonstrated benefit sufficient to justify the
additional complexity.

### `read()` / `write()` for USB Payload Data

USB transfers could be transported entirely through `read()` and
`write()`.

**Decision:** Not selected.

This model is simple, but potentially requires additional copies between
kernel space and user space for large or frequent transfers. Shared
memory provides a more appropriate foundation for high-throughput Bulk
traffic and, in particular, Isochronous traffic.

### `write()` as the User-Space→Kernel Doorbell

User space could notify the kernel after updating the shared-memory ring
using a small `write()` operation.

**Decision:** Not selected.

The notification has command semantics and does not represent a byte
stream. `ioctl()` expresses this intent more explicitly and is therefore
preferred.

### Generic Netlink

The Control Plane could use Generic Netlink.

**Decision:** Not selected.

Generic Netlink provides structured messaging and event mechanisms, but
would introduce another communication interface in addition to the
character device already required for the USB Data Plane. No sufficient
advantage has currently been identified to justify that additional
interface.

### Separate Character Devices for Hubs or Ports

Hubs and ports could be represented by additional `/dev` nodes.

**Decision:** Rejected.

Hubs and ports are objects within an HCD-specific VirtUSB topology and
are addressed within that topology. Separate device nodes would
unnecessarily expose the internal VirtUSB topology through the Linux
device-node model.

------------------------------------------------------------------------

## Consequences

### Advantages

-   clear one-to-one association between `/dev/virtusbX` and a
    `VirtUsbHcd`,
-   a single kernel endpoint for `libvirtusb`,
-   semantic separation between Control Plane and USB Data Plane,
-   reduced copying of USB payload data,
-   efficient integration with `poll()` and `epoll()`,
-   a generic bidirectional ring abstraction,
-   no public ABI dependency on Linux-internal USB structures,
-   no separate Linux device nodes required for hubs and ports.

### Disadvantages

-   shared memory requires explicit synchronization and ownership rules,
-   ring descriptors and payload offsets must be treated as untrusted
    user-space input by the kernel,
-   backend termination and session teardown require defined recovery
    behavior,
-   memory ordering and race conditions require explicit treatment in
    the detailed design,
-   the ABI is more complex than a pure `read()`/`write()` design.

------------------------------------------------------------------------

## Open Issues

This ADR intentionally does not define:

-   concrete `ioctl()` command numbers,
-   concrete ABI structures,
-   the exact number and size of shared-memory rings,
-   payload-buffer size and organization,
-   ring-index types,
-   memory-barrier and synchronization rules,
-   ownership and lifetime of transfer buffers,
-   behavior when a ring is full,
-   behavior when a user-space backend terminates unexpectedly,
-   ownership rules when multiple processes open the same HCD,
-   detailed handling of SOF and microframes,
-   Isochronous scheduling,
-   ABI versioning.

These details will be specified when they become necessary for
implementation.

------------------------------------------------------------------------

## Implementation Consequence

The HCD implementation shall be structured so that the later
kernel--user-space communication layer can follow this model.

The concrete kernel--user-space ABI shall be specified before
Control-Plane operations or USB transfers are first transported across
the user/kernel boundary.
