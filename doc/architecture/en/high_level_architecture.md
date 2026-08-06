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
relationships but do not modify the topology structure.

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
