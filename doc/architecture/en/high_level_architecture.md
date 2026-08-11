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

### Relationship Components ↔ Ports

USB-capable VirtUSB components own their ports as integral subobjects.

- `VirtUsbDev` owns exactly one upstream `VirtUsbPort`.
- `VirtUsbHub` owns exactly one upstream `VirtUsbPort` and one or more
  downstream `VirtUsbPort` instances.
- `VirtUsbRHub` owns downstream `VirtUsbPort` instances only. It has no USB
  upstream port because its upstream side terminates at the host controller.

`VirtUsbPort` is not a VirtUSB Core Component. It has no independent lifetime
or global object identity. A port exists exactly as long as its owning Core
Component exists.

Each port has a role identifying it as an upstream or downstream port.
Role-independent properties and state are represented in the common port
representation. Properties or state that exist only for one role are kept in
role-specific upstream or downstream state.

### Canonical Port State

A `VirtUsbPort` describes only its own local capabilities, properties, and
state. These values are authoritative for that port and are independent of the
values stored by another port.

For example, an upstream and a downstream port each describe their own
supported USB-speed capabilities. Downstream ports additionally describe their
actual local VBUS state.

Properties of the effective USB connection are not stored as a second,
persistent state representation inside `VirtUsbPort`. The port capabilities
constrain the possible connection properties, while the current USB operating
state, including the current operating speed, is determined by the USB
protocol layer according to the applicable USB rules.

The architectural rule is therefore:

> A `VirtUsbPort` stores facts about itself. Effective link and USB-visible
> properties are derived from both ports and the USB rules.


The common port representation contains only virtual hardware and topology
properties. USB-defined protocol state is intentionally kept outside
`VirtUsbPort`.

Current common hardware properties include:

- `speed`: a bit mask of USB speeds supported by the local port hardware;
  the current operating speed is determined by the USB protocol layer and is
  not stored in `VirtUsbPort`.

Downstream-specific hardware state includes:

- `powered`: whether VBUS is physically/logically present at that downstream
  port.
- `over_current`: the local per-port over-current condition, relevant when the
  owning hub uses per-port over-current protection.

USB states such as suspend, connection, enable, reset, and the USB-defined
`PORT_*` and `C_PORT_*` fields belong to the USB protocol/hub layer rather
than to the virtual port hardware model.

Hub-wide bitmaps and other packed representations are likewise not persistent
parallel state. They are generated from the individual `VirtUsbPort` instances
only when required for UAPI transfer, USB hub-class translation, or another
explicit serialization boundary. Incoming aggregate representations are
decoded into the affected port state instead of being stored independently.

### Relationship Port ↔ Port

A virtual USB Attachment is represented locally between exactly one downstream
`VirtUsbPort` and one upstream `VirtUsbPort`.

Each attached port references the other port as its peer. The relationship is
reciprocal and must remain consistent:

```text
downstream.peer -> upstream
upstream.peer   -> downstream
```

There is no separate persistent runtime state for Association and Attachment.
Assigning the two ports to each other is the virtual act of plugging them
together.

Therefore:

```text
peer == NULL     -> detached
peer != NULL     -> attached
```

An upstream port can be attached to at most one downstream port, and a
downstream port can be attached to at most one upstream port.
Upstream-to-upstream and downstream-to-downstream Attachments are invalid.

The term **Association** may be used descriptively for the structural
relationship established by Attachment, but VirtUSB does not support an
independent `associated && !attached` runtime state.

Attachment defines the local parent-child structure of the virtual USB
topology. It does not by itself imply USB-visible Connection or enumeration.
USB-visible Connection is a separate, derived condition based on the local
state of the involved ports and the applicable USB rules.

### Relationship `VirtUsbHub` ↔ Topology

`VirtUsbHub` is functionally a specialized `VirtUsbDev`.

Its upstream port can be attached to at most one downstream port of a
parent `VirtUsbRHub` or `VirtUsbHub`. Its own downstream-port Attachments are
independent of that parent Attachment.

Detaching a `VirtUsbHub` upstream port from its parent downstream port does
not modify the Attachments within the subtree beneath it. The subtree remains
internally intact and can later become part of another VirtUSB topology by
attaching the hub's upstream port to a downstream port in that topology.

### Relationship `VirtUsbDev` ↔ Topology

A `VirtUsbDev` exists independently of any VirtUSB topology.

Its upstream port may be detached. Only when that upstream port is
attached to a downstream port and the resulting Attachment tree forms a
continuous path to exactly one `VirtUsbRHub` does the device become part of an
HCD topology.

HCD membership is therefore derived by traversing the port Attachment tree.
It is not stored as an independent device-to-HCD relationship.

Detaching a device removes only its peer relationship; it does not destroy the device.

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

The following state and relationships are specific to VirtUSB:

- Attachment and Detachment of virtual ports
- backend ownership and lifecycle
- Device Power
- USB Hardware Availability
- optional simulated device-hardware fault or availability conditions

Attachment establishes the local parent-child topology relationship directly
between one downstream `VirtUsbPort` and one upstream `VirtUsbPort`. It is
represented by reciprocal `peer` references and there is no separate
Association state.

The HCD to which an attached object belongs is derived by traversing the
Attachment tree towards the `VirtUsbRHub`. There is no independent
device-to-HCD relationship.

A detached `VirtUsbDev` or `VirtUsbHub` continues to exist independently of
topology membership and may later be attached to any compatible downstream
port.

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

VirtUSB Attachment/topology state is not itself part of this USB device-state machine.

### Device Hardware State

Device hardware state describes conditions of the simulated device that are not
themselves USB protocol states.

Examples include Device Power and USB Hardware Availability.

Device Power represents whether the virtual device hardware is powered. USB
Hardware Availability represents whether the virtual device-side USB controller
and required USB hardware are operational.

These conditions may influence USB-visible behavior but do not replace the
USB-defined device states.

### Canonical Port Representation

`VirtUsbPort` is the single authoritative internal representation of a USB
port. It is used for both upstream and downstream ports.

The common part contains properties and state whose semantics are identical
for both roles. Role-specific state is represented separately for upstream and
downstream ports.

Each port stores only its own local state. Values that describe an effective
connection are calculated from both attached peer ports according to USB rules
and are not redundantly persisted.

Persistent hub-wide status or change bitmaps must not duplicate port state.
Such representations are generated only when required at an interface
boundary.

### Hub Hardware Capabilities

`VirtUsbHub` and `VirtUsbRHub` model the relevant hub hardware capabilities
separately from the current per-port hardware state.

Power switching mode is represented as either:

- **Ganged**: all downstream-port power switches operate together.
- **Individual**: each downstream port can be switched independently.

The historical USB 1.0 no-power-switching mode is not modeled by VirtUSB.

Regardless of switching mode, the actual VBUS state is stored per downstream
`VirtUsbPort`. In ganged mode, a power operation updates all downstream ports
together. A convenience API may expose an explicit "set all ports power"
operation, but this does not introduce an additional hub-wide power state.

Over-current protection mode is represented as one of:

- **Global**: the hub reports one combined over-current condition.
- **Per-port**: each downstream port has its own over-current condition.
- **None**: no over-current protection is modeled.

The current global over-current condition is hub-local hardware state. The
current per-port over-current condition is downstream-port-local hardware
state. Only the state corresponding to the configured protection mode is
semantically active.

The USB hub protocol layer maps these hardware capabilities and conditions to
the corresponding USB 2.0 hub descriptor, status, and change fields.

### USB Hub-Port State

The USB-defined hub-port state is a protocol-layer representation. It is
not stored as hardware bits inside `VirtUsbPort`.

`PORT_CONNECTION`, `PORT_ENABLE`, `PORT_SUSPEND`, `PORT_OVER_CURRENT`,
`PORT_RESET`, `PORT_POWER`, speed status, and the corresponding `C_PORT_*`
change fields are maintained or derived by the USB hub protocol/state-machine
layer as required by USB 2.0 semantics.


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

`PORT_POWER` is the USB-defined protocol-layer representation of downstream
port power. It is distinct from Device Power. The actual virtual-hardware VBUS
state is stored as the downstream `VirtUsbPort.powered` state; the USB hub
protocol layer maps the hardware state and hub power-switching semantics to the
USB-defined `PORT_POWER` status.

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
Device object exists
        |
        v
Device attached to downstream port
(reciprocal peer relationship)
        +
Device power / USB hardware conditions permit operation
        +
Downstream VBUS / USB hub protocol state permits detection
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
