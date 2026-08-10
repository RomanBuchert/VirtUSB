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

A downstream port can be associated with at most one `VirtUsbDev` or
`VirtUsbHub`. A `VirtUsbDev` or `VirtUsbHub` can be associated with at
most one downstream port at any time.

Association establishes the parent-child relationship between the hub
port and the device. It defines the local structure of the VirtUSB
topology but does not imply that the device is attached or USB-visible.

### Relationship `VirtUsbHub` ↔ Topology

`VirtUsbHub` is functionally a specialized `VirtUsbDev`.

A `VirtUsbHub` can be associated with at most one downstream port. In
contrast, a `VirtUsbRHub` has no parent port.

Disassociating a `VirtUsbHub` from its parent port does not modify the
associations within the subtree beneath it. The subtree remains
internally intact and can later become part of another VirtUSB topology
by associating its root hub object with a downstream port of that
topology.

### Relationship `VirtUsbDev` ↔ Topology

A `VirtUsbDev` exists independently of any VirtUSB topology.

Only after Association establishes a parent-child relationship with a
hub port and that relationship forms a continuous path to exactly one
`VirtUsbHcd` does the device become part of that HCD's topology.

HCD membership is therefore derived from the Association tree. It is
not stored as an independent device-to-HCD relationship.

Detaching a device does not affect its Association or its existence.

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

## State and Operation Model

VirtUSB aligns its externally visible USB state with the terminology and
semantics defined by the USB specification.

VirtUSB-specific state is introduced only where the project requires a
management or virtual-hardware concept for which USB defines no equivalent
state.

This distinction prevents internal control state from being confused with
USB-visible device or hub-port state.

### VirtUSB-Specific Management State

The following relationships are specific to VirtUSB:

- Association and Disassociation
- backend ownership and lifecycle
- USB Hardware Availability
- optional simulated hardware fault or availability conditions

Association assigns a `VirtUsbDev` or `VirtUsbHub` to one specific downstream
port of a `VirtUsbRHub` or `VirtUsbHub`. It establishes the local parent-child
relationship and therefore defines the structure of the VirtUSB topology.
Association does not by itself mean that the device is attached or USB-visible.

The HCD to which an associated object belongs is derived by traversing its
parent relationships towards the `VirtUsbRHub`. There is no independent
device-to-HCD Association. Consequently, moving an associated hub subtree to a
different topology requires changing only the Association of the subtree root;
the Associations within that subtree remain unchanged.

Association and Attachment are therefore distinct:

```text
Virtual device exists
        |
        v
Associated with one downstream port
(topology position defined)
        |
        v
Attached at that associated port
```

A device may remain associated while detached and may later be attached again
at its associated port without recreating the device or backend.

### USB Device State

Once a device is attached, VirtUSB follows the USB-defined visible device-state
model as closely as practical.

The relevant USB-defined states are:

- Attached
- Powered
- Default
- Address
- Configured
- Suspended

These states retain their USB specification meaning.

An attached device may exist without being powered. After power is available,
the device is in the Powered state but must not respond to normal bus
transactions until it has received reset signaling. Completion of reset places
the device in Default. Address assignment and configuration then move it
through Address and Configured according to normal USB enumeration.

VirtUSB-specific Association is not part of this USB device-state machine.

### Device Hardware State

Device hardware state describes conditions of the simulated device that are not
themselves USB protocol states.

Examples include Device Power and USB Hardware Availability.

Device Power represents whether the virtual device hardware is powered. USB
Hardware Availability represents whether the virtual device-side USB controller
and required USB hardware are operational.

These conditions may influence USB-visible behavior but do not replace the
USB-defined device states.

### USB Hub-Port State

Downstream ports follow the USB-defined hub-port state model wherever practical.

Relevant states include:

- Powered-off
- Disconnected
- Disabled
- Resetting
- Enabled
- Suspended
- Resuming

The USB-visible status of a downstream port is represented by the corresponding
USB-defined status fields, including:

- `PORT_CONNECTION`
- `PORT_ENABLE`
- `PORT_SUSPEND`
- `PORT_OVER_CURRENT`
- `PORT_RESET`
- `PORT_POWER`

`PORT_POWER` is the USB-defined logical port-power state. It is not an alias for
Device Power and does not automatically imply a separately modeled physical
power-supply condition.

### Attachment and USB Connection

Attachment and USB Connection are related but are not interchangeable control
variables.

Attach establishes that a virtual device is present at a concrete downstream
port.

USB Connection is the host-visible `PORT_CONNECTION` status of that port.

For a port whose USB-defined Port Power state permits detection,
`PORT_CONNECTION` reflects whether the attached device is detected. A port in
the USB-defined `Powered-off` or `Disconnected` state reports no connection.

The relationship is therefore conceptually:

```text
Device associated with controller
        |
        v
Device attached to port
        +
Device power / USB hardware conditions permit operation
        +
USB hub port is not Powered-off
        |
        v
Attachment is detectable
        |
        v
PORT_CONNECTION = 1
        |
        v
C_PORT_CONNECTION on transition
        |
        v
Host enumeration
```

`PORT_CONNECTION` is therefore derived state and must not be modeled as an
arbitrary user-space writable switch.

Likewise, clearing `PORT_CONNECTION` is a consequence of a USB-defined port
state or of removing a prerequisite such as attachment; it is not an
independent Detach operation.

### USB Change State

USB change state records USB-defined port changes that have not yet been
acknowledged by the host.

Examples include connection, enable, suspend, over-current, and reset changes.

These change bits are derived from the corresponding USB-visible transitions.
They are not independent virtual-hardware state.

### Enumeration

Enumeration remains a USB host operation.

When a device attachment is detected on a powered port, the hub reports the
change to the host. The host identifies the changed port, waits for the required
connection stabilization interval, resets and enables the port, and then
progresses the device through the USB-defined Default, Address, and Configured
states.

VirtUSB provides the virtual controller, hub, topology, and backend interaction
required to support this process. The Linux USB subsystem remains responsible
for normal host-side enumeration behavior.

### Operation Categories

VirtUSB distinguishes three categories of state transition.

**VirtUSB management operations** manipulate project-specific relationships or
virtual hardware, for example:

- Associate
- Disassociate
- Attach
- Detach
- Device Power changes
- USB Hardware Availability changes
- simulated hardware fault conditions

**Derived USB-visible transitions** result from the state model rather than from
direct Control Plane commands, for example:

- `PORT_CONNECTION` becoming set or clear
- corresponding USB port-change indications

**USB protocol and hub operations** remain governed by USB semantics and the
host stack, for example:

- Port Power control
- Port Reset
- Port Enable/Disable
- Suspend/Resume
- Enumeration
- Address assignment
- Configuration

State transitions may propagate between these domains, but their meanings
remain separate.

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
