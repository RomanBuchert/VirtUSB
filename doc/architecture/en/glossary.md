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

# System Operations

System operations describe fundamental operations that modify either the
state of a virtual USB device or its integration into the virtual USB
infrastructure.

## Power On

Power On activates a virtual USB device at the Device Layer.

A powered-on device is operational. However, it does not need to be
attached to a USB port or have an initialized USB Device Controller.

## Power Off

Power Off deactivates a virtual USB device at the Device Layer.

A powered-off device can neither have an initialized USB Device
Controller nor participate in USB protocol communication.

## Attach

Attach assigns a virtual USB device to a virtual USB port.

Attaching modifies only the USB topology and does not yet establish a
USB connection to the host.

## Detach

Detach removes the association between a virtual USB device and a
virtual USB port.

After detaching, the device is no longer connected to the virtual USB
topology.

## USB Connect

USB Connect signals the presence of a virtual USB device to the virtual
USB Host Controller.

The virtual USB Device Controller must already be initialized.

USB Connect requires the device to be attached to a virtual USB port.

USB Connect does not initiate enumeration by itself, but makes it
possible.

## USB Disconnect

USB Disconnect terminates the USB connection to the virtual USB Host
Controller as signaled by the virtual USB Device Controller.

The virtual USB device remains part of the virtual USB topology and may
remain attached to a virtual USB port.

## Enumeration

Enumeration describes the process initiated by the USB host to detect,
identify, and configure a newly connected USB device.

Enumeration is initiated by the USB host after detecting a USB Connect
event and includes, among other things, reading the USB descriptors,
assigning a USB address, and configuring the USB device.

# Core Components

Core components are the virtual counterparts of the fundamental hardware
components of a physical USB infrastructure.

They form the foundation of the virtual USB topology and perform the
same fundamental roles within VirtUSB as their physical counterparts.
Their implementation may differ from the underlying hardware.

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
