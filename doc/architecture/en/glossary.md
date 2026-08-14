# VirtUSB Glossary

# Architecture Layers

Architecture layers describe different conceptual abstraction layers of
a virtual USB device.

Each architecture layer has a clearly defined responsibility and is
independent of its specific implementation.

## Device Layer

The **Device Layer** describes all device-specific properties, states,
and operations of a virtual USB device, independent of its integration
into a VirtUSB topology.

It has no knowledge of the USB topology, the state of the USB Device
Controller, or the current USB protocol state.

## USB Topology Layer

The **USB Topology Layer** describes the structural organization of a
VirtUSB topology and the parent-child relationships between its core
components.

It defines which core components are part of a VirtUSB topology and how
they are interconnected.

It has no knowledge of device-specific states, the state of the USB
Device Controller, or USB protocol communication.

## USB Device Controller Layer

The **USB Device Controller Layer** describes the state and control of
the virtual USB Device Controller of a virtual USB device.

It forms the interface between the Device Layer and the USB Protocol
Layer.

It has no knowledge of the device-specific state or the USB topology.

## USB Protocol Layer

The **USB Protocol Layer** describes all communication procedures
defined by the USB specification between the USB host and USB devices.

It has no knowledge of the device-specific state or the USB topology.

# State and Operation Terminology

VirtUSB follows USB specification terminology wherever the specification
already defines the relevant externally visible state or operation.

Project-specific terminology is introduced only for concepts that are required
by VirtUSB itself and have no equivalent USB-defined meaning.

In particular, USB-defined device states, hub-port states, and port-status
fields retain their USB meaning. VirtUSB management state must not redefine
those terms.

## Association

**Association** is a descriptive term for the structural relationship created
by attaching one downstream `VirtUsbPort` to one upstream `VirtUsbPort`.

VirtUSB does not maintain Association and Attachment as separate runtime
states. The reciprocal `peer` references are the Attachment:

```text
peer == NULL     -> detached
peer != NULL     -> attached
```

Consequently, there is no `associated && !attached` state.

The Attachment defines the parent-child structure of a VirtUSB topology but
does not by itself imply USB-visible Connection or enumeration.

The HCD to which a device or hub subtree belongs is derived by traversing the
Attachment tree toward the `VirtUsbRHub`; it is not stored as a separate
device-to-HCD relationship.

Detaching the upstream port of a `VirtUsbHub` does not alter Attachments inside
the subtree below that hub.


## Attachment

**Attachment** is the virtual act and resulting runtime state of plugging one
upstream `VirtUsbPort` into one downstream `VirtUsbPort`.

It is represented exclusively by reciprocal `peer` references between the two
ports. No additional `attached` flag is stored.

Attachment is distinct from USB-visible Connection. Attached ports can remain
not connected from the USB protocol point of view if their local state and the
USB rules do not permit an effective connection.



## Attach

Attach establishes a virtual USB topology relationship by plugging one
upstream `VirtUsbPort` into one downstream `VirtUsbPort`.

The resulting Attachment is represented exclusively by reciprocal `peer`
references. There is no separate Association state and no additional
`attached` flag.

Attach alone does not imply that the host can already detect the device.
Downstream VBUS state, device hardware conditions, and USB protocol state still
determine whether the attachment becomes USB-visible.

## Detach

Detach removes the reciprocal `peer` relationship between an upstream and a
downstream `VirtUsbPort`.

If the device was visible to the host, detaching it necessarily causes the
corresponding USB-visible connection to disappear. Detach does not destroy the
virtual device, hub, or its backend. A detached Core Component continues to
exist independently and may later be attached elsewhere.

## Device-Side Connection Signaling

Device-Side Connection Signaling is the upstream-port hardware state that
describes whether a virtual device currently signals USB presence to its
attached downstream peer.

It is independent of Attachment. A device can remain attached while connection
signaling is disabled.

The device implementation controls this state through the VirtUSB device-side
interface. Internal device conditions such as self-power, firmware state,
controller state, or simulated faults remain outside VirtUSB.

This state is intended to correspond to device-controller connect/disconnect
operations and is one input to USB-visible connection detection.

## Port Power

Port Power refers to the USB-defined protocol-layer power status represented by
`PORT_POWER`.

The actual virtual-hardware VBUS state of a downstream port is stored in that
`VirtUsbPort` as `powered`.

The hub power-switching capability defines whether downstream VBUS is switched
individually or ganged. The USB hub protocol layer maps this hardware state and
switching behavior to the USB-defined `PORT_POWER` status.

## USB Connection

USB Connection is the host-visible port connection represented by the
USB-defined `PORT_CONNECTION` status.

It is not an independently writable VirtUSB topology state.

For a powered port, `PORT_CONNECTION` reflects whether an attached device is
detected. In the USB-defined `Powered-off` or `Disconnected` port states,
`PORT_CONNECTION` is clear.

VirtUSB therefore derives USB Connection from Attachment, downstream VBUS,
device-side connection signaling, and the applicable USB hub-port state instead
of exposing `PORT_CONNECTION` as an arbitrary user-space switch.

## USB Disconnect

USB Disconnect is the loss of the USB-visible connection represented by
`PORT_CONNECTION`.

It may result from Detach, loss of the conditions required for connection
detection, a USB-defined port-state transition, or another condition that
causes the port to stop reporting an attached device.

A USB Disconnect does not necessarily imply Detach.

## USB Device States

VirtUSB uses the USB-defined visible device states with their specification
meaning:

- Attached
- Powered
- Default
- Address
- Configured
- Suspended

These are USB device states and are distinct from VirtUSB-specific topology
and device-side connection-signaling state.

The USB-defined Attached state begins when the device is attached to the USB.
The Powered state additionally requires power. Reset moves a powered device to
Default, address assignment moves it to Address, and successful configuration
moves it to Configured.

## Hub Port States

VirtUSB models downstream hub-port behavior using USB-defined port semantics
wherever practical.

Relevant USB-defined downstream port states include, among others:

- Powered-off
- Disconnected
- Disabled
- Resetting
- Enabled
- Suspended
- Resuming

The corresponding USB port-status fields such as `PORT_CONNECTION`,
`PORT_ENABLE`, `PORT_SUSPEND`, `PORT_RESET`, `PORT_OVER_CURRENT`, and
`PORT_POWER` describe this USB-visible state and are not generic VirtUSB
hardware-control variables.

## Enumeration

Enumeration is the USB-defined host process used to identify and configure an
attached device.

When an attachment is detected on a powered port, the host observes the port
change, queries the hub, performs the required reset sequence, assigns an
address, reads descriptors, and may configure the device.

Enumeration is driven by the USB host. VirtUSB supplies the virtual hub,
topology, and device behavior required for that process but does not replace
the USB-defined enumeration state machine.

# Core Components

Core components are the virtual counterparts of the fundamental hardware
components of a physical USB infrastructure.

They form the foundation of the virtual USB topology and perform the
same fundamental roles within VirtUSB as their physical counterparts.
Their implementation may differ from the underlying hardware.

## VirtUsbPort

A **VirtUsbPort** is the internal representation of one virtual USB port.

The same port type is used for upstream and downstream ports. Its role
identifies which hardware semantics apply.

A `VirtUsbPort` is not a VirtUSB Core Component and has no independent
lifetime or global object identity. It is an integral subobject of its owning
Core Component.

A `VirtUsbDev` owns one upstream port. A `VirtUsbHub` owns one upstream port
and one or more downstream ports. A `VirtUsbRHub` owns downstream ports only.

`VirtUsbPort` contains only topology and virtual-hardware information. USB
2.0 protocol state is intentionally maintained outside the port hardware
object.

Common properties include the reciprocal `peer` relationship and the local
USB speed capability mask. A downstream port additionally contains its actual
VBUS/power state and, when applicable, its per-port over-current hardware
condition.

The `speed` property is a bit mask of speeds supported by the local hardware.
The current USB operating speed is determined by the USB protocol layer and is
not stored in `VirtUsbPort`.

USB-defined states such as connection, enable, suspend, reset, `PORT_*`
status fields, and `C_PORT_*` change fields are not hardware properties of
`VirtUsbPort`.

Aggregate bitmaps or packed representations are derived only when required
for transfer or protocol translation and are not maintained as a second
authoritative copy of hardware state.

## Hub Power Switching Mode

The **Hub Power Switching Mode** is a hardware capability of a `VirtUsbHub` or
`VirtUsbRHub`.

VirtUSB models the USB 2.0 modes **Ganged** and **Individual**. The historical
USB 1.0 no-power-switching mode is not modeled.

The capability controls how downstream-port power operations affect the
individual `VirtUsbPort.powered` hardware states. It does not create a
separate hub-wide power-state variable.

## Hub Over-Current Mode

The **Hub Over-Current Mode** is a hardware capability of a `VirtUsbHub` or
`VirtUsbRHub`.

VirtUSB models **Global**, **Per-port**, and **None**.

In Global mode, the current over-current condition belongs to the hub hardware
state. In Per-port mode, each downstream `VirtUsbPort` has its own
over-current hardware condition. The USB protocol layer translates this
hardware model into the corresponding USB 2.0 hub or port status.


## Hub Status Change Notification

A **Hub Status Change Notification** is a generic notification emitted by the
common `VirtUsbHub` model when new USB-defined hub or port change information
becomes pending.

The notification does not define how the change reaches the host. A
`VirtUsbRHub` maps it through its `VirtUsbHcd` to the host controller's root-hub
status-notification mechanism. A regular `VirtUsbHub` maps the same semantic
event to its USB hub interrupt-IN endpoint.

A virtual-hardware state change, a USB-defined change condition, and a Hub
Status Change Notification are distinct concepts. The notification indicates
new pending USB change information; it is not a generic notification for every
hardware-state update.

## VirtUsbHcd

A **VirtUsbHcd** (Virtual USB Host Controller) is the virtual
counterpart of a physical USB Host Controller.

It provides the virtual USB Root Hub (`VirtUsbRHub`) and forms the
interface between the host operating system and the virtual USB
topology.

## VirtUsbRHub

A **VirtUsbRHub** (Virtual USB Root Hub) is the virtual counterpart of
the root hub of a physical USB Host Controller.

It belongs directly to a `VirtUsbHcd` and provides downstream
ports to which virtual USB devices (`VirtUsbDev`) or additional virtual
USB hubs (`VirtUsbHub`) can be attached.

## VirtUsbHub

A **VirtUsbHub** (Virtual USB Hub) is the virtual counterpart of a
physical USB hub.

It extends the virtual USB topology with additional downstream ports to
which virtual USB devices (`VirtUsbDev`) or additional virtual USB hubs
(`VirtUsbHub`) can be attached.

## VirtUsbDev

A **VirtUsbDev** (Virtual USB Device) is the virtual counterpart of a
physical USB device.

It provides the behavior of the corresponding USB device to the USB host
and can be attached to a virtual USB port.

# Project Documents

This chapter defines the purpose of the project documentation.

Each project document answers exactly one question. Documents at lower
levels refine the content of higher-level documents without leaving
their level of abstraction.

## System Overview

**Question:**

> What is VirtUSB?

The System Overview describes the purpose, scope, and fundamental
architectural principles of VirtUSB.

It serves as the entry point to the project and is intentionally kept
concise. Detailed descriptions belong exclusively in subsequent
architecture documents.

## Glossary

**Question:**

> What do the terms used throughout the project mean?

The Glossary defines the terminology used throughout the project as well
as the purpose of the project documents.

It does not describe architecture or implementation details.

## High-Level Architecture

**Question:**

> What are the core components of VirtUSB and how are they related?

The High-Level Architecture elaborates on the System Overview.

It describes the overall structure of VirtUSB, the relationships between
the core components, and their fundamental responsibilities.

It provides the foundation for subsequent architecture documents.

Subsequent documents refine individual aspects of the High-Level
Architecture but must not contradict its fundamental architectural
decisions.
