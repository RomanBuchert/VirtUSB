# High-Level Architecture

## Purpose

**Question:**

> What are the core components of VirtUSB and how are they related?

The High-Level Architecture elaborates on the System Overview.

It describes the overall structure of VirtUSB, the relationships between
its core components, and their fundamental responsibilities.

It describes the architecture exclusively at a high level of abstraction
and contains no implementation details.

It provides the foundation for subsequent architecture documents, which
further refine individual aspects of the High-Level Architecture.

## Fundamental System Structure

VirtUSB provides a fully virtual USB infrastructure whose structure is
based on that of a physical USB system.

A VirtUSB topology consists exclusively of virtual components. Mixing
virtual and physical USB components within the same VirtUSB topology is
not intended.

Multiple `VirtUsbHcd` instances may exist simultaneously. Each
`VirtUsbHcd` forms the root of its own VirtUSB topology and owns exactly
one `VirtUsbRHub`.

The fundamental core components of VirtUSB are:

-   `VirtUsbHcd`
-   `VirtUsbRHub`
-   `VirtUsbHub`
-   `VirtUsbDev`

Their conceptual meaning is defined in the Glossary.

### Example of a VirtUSB Topology

``` mermaid
flowchart TD
    HCD["VirtUsbHcd"]
    RHUB["VirtUsbRHub"]

    DEV1["VirtUsbDev A"]
    HUB1["VirtUsbHub A"]
    DEV2["VirtUsbDev B"]
    HUB2["VirtUsbHub B"]
    DEV3["VirtUsbDev C"]

    HCD --> RHUB
    RHUB -->|"Downstream Port"| DEV1
    RHUB -->|"Downstream Port"| HUB1
    HUB1 -->|"Downstream Port"| DEV2
    HUB1 -->|"Downstream Port"| HUB2
    HUB2 -->|"Downstream Port"| DEV3
```

The diagram illustrates the fundamental structure of a VirtUSB topology.
The relationships and architectural rules are described in the following
sections.

## Core Component Relationships

The relationships between the core components define the structure of a
VirtUSB topology.

A VirtUSB topology forms a parent-child tree whose root consists of a
`VirtUsbHcd` and its `VirtUsbRHub`.

### Relationship `VirtUsbHcd` ↔ `VirtUsbRHub`

Each `VirtUsbHcd` owns exactly one `VirtUsbRHub`. Each `VirtUsbRHub`
belongs to exactly one `VirtUsbHcd`.

### Relationship Hub ↔ Downstream Ports

Both `VirtUsbRHub` and `VirtUsbHub` provide downstream ports. Downstream
ports are integral parts of their respective hubs and have no
independent existence.

### Relationship Downstream Port ↔ `VirtUsbDev`

A downstream port can have at most one `VirtUsbDev` attached. A
`VirtUsbDev` can be attached to at most one downstream port at any time.

Attaching a device establishes a parent-child relationship between the
hub and the device.

### Relationship `VirtUsbHub` ↔ Topology

`VirtUsbHub` is functionally a specialized `VirtUsbDev`.

A `VirtUsbHub` can be attached to at most one downstream port. In
contrast, a `VirtUsbRHub` has no parent port.

Detaching a `VirtUsbHub` does not modify the parent-child structure of
the subtree beneath it. The subtree remains internally intact and can
later become part of a VirtUSB topology again.

### Relationship `VirtUsbDev` ↔ Topology

A `VirtUsbDev` exists independently of any VirtUSB topology.

Only after establishing a parent-child relationship with a hub port and
having a continuous path to exactly one `VirtUsbHcd` does it become part
of a VirtUSB topology.

Detaching a device does not affect its existence.

### Cascading

VirtUSB does not introduce any additional artificial limitation on hub
cascading.

Only the limits defined by the USB specification and the respective
implementation apply.

State changes may propagate recursively along existing parent-child
relationships but never modify the structure of the subtree.

### Component Relationships

``` mermaid
flowchart TD
    HCD["VirtUsbHcd"]
    RHUB["VirtUsbRHub"]
    HUB["VirtUsbHub"]
    PORT["Downstream Port"]
    DEV["VirtUsbDev"]

    HCD -->|"owns"| RHUB
    RHUB -->|"contains"| PORT
    HUB -->|"contains"| PORT
    PORT -->|"connects"| DEV

    HUB -.->|"functionally specialized device"| DEV
```

## Architecture Layers

Architecture layers describe different conceptual views of the same core
components. They are not software layers or implementation layers.

-   Device Layer
-   USB Topology Layer
-   USB Device Controller Layer
-   USB Protocol Layer

Each core component has states and properties on all four architecture
layers.

## System Operations

System operations modify the state of a VirtUSB topology.

These include:

-   Power On
-   Power Off
-   Attach
-   Detach
-   USB Connect
-   USB Disconnect
-   Enumeration

The meaning of each system operation is defined in the Glossary.

System operations may propagate recursively along existing parent-child
relationships. Attach and Detach are the operations that establish or remove
the corresponding parent-child relationship; other state changes do not modify
the topology structure.

## State Model

VirtUSB distinguishes state according to the architectural domain in which the
state exists. Similar terms in different domains are not aliases and must not
be combined merely because one state may cause another.

### Device Hardware State

Device hardware state describes the operational state of the virtual device
itself. It includes whether the device is powered on and whether its virtual USB
Device Controller is operational.

This state exists independently of the device's integration into a VirtUSB
topology. A virtual device may exist and be powered on without being attached
to a port.

### Topology State

Topology state describes relationships between VirtUSB components. In
particular, it records whether a `VirtUsbDev` is associated with and attached to
a downstream port.

Attach and Detach modify this relationship. Attachment alone does not imply a
host-visible USB connection.

### Port Hardware State

Port hardware state represents externally controllable or simulated conditions
of the virtual port hardware. Examples include power availability and
over-current conditions. Connection speed may also originate from the virtual
hardware or attached device model.

Port hardware state is distinct from the USB hub-class status reported to the
host.

### USB-Visible Port State

USB-visible port state is the state exposed through USB hub semantics. It
includes connection, enable, suspend, reset, logical port power, over-current,
and the speed information applicable to the connected device.

Some USB-visible states are consequences of topology and virtual-hardware
conditions. Others are controlled by normal USB host and hub operation. They
are therefore not generally writable Control Plane properties.

In particular, host-visible connection is derived from the conditions required
for the port to detect an attached device. A typical relationship is:

```text
Device attached to port
        +
Device-side USB hardware operational
        +
Port hardware permits operation
        +
USB logical Port Power active
        |
        v
Host-visible USB connection
```

If one of the required conditions is removed, the host-visible connection must
no longer be reported even if the device remains attached to the topology.

### USB Change State

USB change state records host-visible changes that have not yet been
acknowledged according to USB hub semantics. Examples include connection,
enable, suspend, reset, and over-current changes.

Change state is derived from transitions of the corresponding USB-visible state
and is not an independent virtual-hardware control surface.

### USB Device Protocol State

The USB device protocol state machine, including the USB-defined states
Attached, Powered, Default, Address, Configured, and Suspended, is separate from
the VirtUSB topology and port state described above.

In particular, the USB-defined state named Attached must not be confused with
the VirtUSB Attach operation. VirtUSB uses Attach to describe the topology
relationship between a virtual device and a downstream port.

### State Propagation

State transitions may propagate between domains, but the originating and
derived states remain distinct. For example, attaching a device does not
directly set the USB connection state. Instead, the topology change contributes
to the conditions from which the host-visible connection state is derived.

This separation is also the basis for the Control Plane: user space controls
external virtual-hardware and topology conditions, while USB protocol and
host-controller state remain governed by their respective USB operations.

## Interface and Communication Model

Each core component exposes a well-defined interface.

Communication takes place exclusively along existing parent-child
relationships and is divided into two independent communication paths:

-   Handling
-   USB Communication

Handling includes all activities outside USB communication, in
particular topology and state changes.

USB communication comprises only the communication procedures defined by
the USB specification.

Both communication paths are modeled after the behavior of physical USB
hardware.

## USB Abstraction

VirtUSB does not emulate physical USB hardware.

In particular, it does not emulate:

-   Electrical signaling
-   Bit level
-   Packet level

Communication takes place at a higher level of USB abstraction.

## Fundamental Architecture Rules

-   Multiple `VirtUsbHcd` instances are supported.
-   Exactly one `VirtUsbRHub` exists for each `VirtUsbHcd`.
-   `VirtUsbHub` is functionally a specialized `VirtUsbDev`.
-   `VirtUsbRHub` and `VirtUsbHub` should use the same functional hub
    model whenever possible.
-   Hubs may be cascaded in accordance with the USB specification.
-   A `VirtUsbDev` can be attached to only one downstream port.
-   The existence of a core component is independent of its integration
    into a VirtUSB topology.
-   State changes never modify the parent-child structure.
-   Handling and USB communication are separate communication paths.
-   Each core component is controlled exclusively through its defined
    interface.
