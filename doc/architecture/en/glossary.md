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

Association is a VirtUSB-specific management relationship.

Association is the local logical assignment of a `VirtUsbDev` or `VirtUsbHub`
to exactly one downstream port of a `VirtUsbRHub` or `VirtUsbHub`. It establishes
the parent-child relationship and therefore defines the object's position in a
VirtUSB topology. A downstream port can be associated with at most one such
object.

Association alone does not mean that the associated object is attached or
USB-visible and has no USB-defined meaning.

Membership in a `VirtUsbHcd` topology is derived by traversing the Association
tree towards the `VirtUsbRHub`; it is not represented by an independent
device-to-HCD Association. A locally associated subtree whose root has no path
to a `VirtUsbRHub` therefore belongs to no HCD topology.

## Disassociation

Disassociation removes the VirtUSB-specific Association between a virtual
device or hub and its parent downstream port.

An attached object must be detached before this Association can be removed.
Disassociating the root of a hub subtree does not modify the Associations within
that subtree.

## Attach

Attach makes an associated virtual USB device or hub present at its associated
downstream port from the virtual hardware perspective.

VirtUSB uses the term Attach in the USB sense as closely as practical. Attach
does not define the topology position itself; that position is defined by
Association.

Attach alone does not imply that the host can already detect the device. Port
power state, device power state, and the availability of the virtual USB
hardware still determine whether the attachment becomes USB-visible.

## Detach

Detach removes a virtual USB device from its downstream port.

If the device was visible to the host, detaching it necessarily causes the
corresponding USB-visible connection to disappear. Detach does not by itself
destroy the virtual device or its backend and does not necessarily remove its
VirtUSB Association.

## Device Power

Device Power describes whether the virtual device hardware is powered.

This is distinct from the USB hub-port `PORT_POWER` status and from the
VirtUSB-specific Association of the device.

A device may be associated and attached while not powered, corresponding to the
USB-defined possibility of an Attached but not Powered device.

## USB Hardware Availability

USB Hardware Availability is a VirtUSB-specific device-hardware condition.

It describes whether the virtual device-side USB controller and the device
hardware required for USB communication are operational.

USB Hardware Availability is not a USB-defined device state. It is an internal
or externally simulated prerequisite from which USB-visible behavior may be
derived.

A powered-off device cannot have USB Hardware Availability. A powered device
may nevertheless have unavailable USB hardware if the represented virtual
hardware model requires that distinction.

## Port Power

Port Power refers to the USB-defined logical power-control state represented by
`PORT_POWER`.

It reflects whether a downstream port is in the USB hub state `Powered-off` or
is not in that state. It must not be treated as an alias for Device Power or for
a separate simulated physical power-supply condition.

Where a virtual hardware model requires additional power-supply behavior, that
behavior is modeled as a separate VirtUSB-specific hardware condition.

## USB Connection

USB Connection is the host-visible port connection represented by the
USB-defined `PORT_CONNECTION` status.

It is not an independently writable VirtUSB topology state.

For a powered port, `PORT_CONNECTION` reflects whether an attached device is
detected. In the USB-defined `Powered-off` or `Disconnected` port states,
`PORT_CONNECTION` is clear.

VirtUSB therefore derives USB Connection from the relevant attachment, device
hardware, and USB hub-port state instead of exposing it as an arbitrary
user-space switch.

## USB Disconnect

USB Disconnect is the loss of the USB-visible connection represented by
`PORT_CONNECTION`.

It may result from Detach, loss of the conditions required for connection
detection, a USB-defined port-state transition, or another condition that
causes the port to stop reporting an attached device.

A USB Disconnect does not necessarily imply Detach or Disassociation.

## USB Device States

VirtUSB uses the USB-defined visible device states with their specification
meaning:

- Attached
- Powered
- Default
- Address
- Configured
- Suspended

These are USB device states and are distinct from VirtUSB-specific management
state such as Association and USB Hardware Availability.

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

A **VirtUsbPort** is the internal representation of one downstream port of a
`VirtUsbRHub` or `VirtUsbHub`.

It is not a VirtUSB Core Component and has no independent lifetime or global
object identity. A `VirtUsbPort` exists exactly as long as its parent hub.

The port is the canonical internal location for its topology relationship and
port-local state. Aggregate bitmaps or packed port-state representations are
derived from the individual ports only when required for transfer or protocol
translation; they are not maintained as a second authoritative copy of the
state.

## VirtUsbHcd

A **VirtUsbHcd** (Virtual USB Host Controller) is the virtual
counterpart of a physical USB Host Controller.

It provides the virtual USB Root Hub (`VirtUsbRHub`) and forms the
interface between the host operating system and the virtual USB
topology.

## VirtUsbRHub

A **VirtUsbRHub** (Virtual USB Root Hub) is the virtual counterpart of
the root hub of a physical USB Host Controller.

It is directly associated with a `VirtUsbHcd` and provides downstream
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
