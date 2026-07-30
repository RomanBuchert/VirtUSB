# VirtUSB System Requirements

## 1. Purpose

This document defines the system requirements for VirtUSB.

The requirements describe what the system shall provide without defining how
these requirements are implemented.

Implementation details are documented separately in the architecture
documentation and Architecture Decision Records (ADRs).

---

## 2. Scope

VirtUSB is a virtual USB Host Controller for Linux.

Its purpose is to enable the development and testing of USB devices without
requiring physical USB hardware.

The project consists of:

- a Linux kernel module implementing one or more virtual USB Host Controllers
- a userspace interface for controlling the virtual host controllers
- backend implementations representing virtual USB devices

---

## 3. Terminology

| Term | Description |
|------|-------------|
| Controller | One virtual USB Host Controller instance |
| Root Hub | Virtual USB Root Hub provided by a controller |
| Port | One downstream Root Hub port |
| Backend | Software implementing a virtual USB device |
| Device | USB device presented by a backend |

---

# 4. Functional Requirements

## VUSB-FR-001 — Multiple Controllers

The kernel module shall support multiple independent virtual USB Host
Controller instances.

The number of controller instances shall be configurable when loading the
kernel module.

---

## VUSB-FR-002 — Character Device

Each virtual controller shall expose one character device.

The device naming scheme shall be:

```
/dev/virtusbX
```

where *X* is the controller number.

---

## VUSB-FR-003 — Root Hub

Each controller shall provide exactly one virtual USB Root Hub.

---

## VUSB-FR-004 — Root Hub Ports

Each Root Hub shall provide 31 downstream ports.

---

## VUSB-FR-005 — USB Enumeration

Devices connected through VirtUSB shall enumerate using the standard Linux USB
subsystem.

Enumerated devices shall be visible using standard Linux USB tools.

Example:

```
lsusb
```

---

## VUSB-FR-006 — USB Transfer Types

VirtUSB shall support:

- Control Transfers
- Bulk Transfers
- Interrupt Transfers
- Isochronous Transfers

---

## VUSB-FR-007 — Backend Independence

The kernel module shall not depend on a specific backend implementation.

---

## VUSB-FR-008 — USB Device Framework

VirtUSB shall provide the transport mechanisms required for a backend to
implement a USB device compliant with the USB 2.0 Device Framework.

VirtUSB itself shall not implement device-specific USB functionality.

---

# 5. Interface Requirements

## VUSB-IR-001

Communication between userspace and the kernel module shall be performed using
the controller character device.

---

## VUSB-IR-002

The userspace interface shall support attaching and detaching virtual USB
devices.

---

# 6. Non-Functional Requirements

## VUSB-NFR-001

VirtUSB shall operate on Linux.

---

## VUSB-NFR-002

The kernel module shall be installable using DKMS.

---

## VUSB-NFR-003

The project shall compile with both GCC and LLVM/Clang.

---

## VUSB-NFR-004

Project documentation shall be written in Markdown.

---

# 7. Explicit Non-Goals

The following features are explicitly outside the scope of VirtUSB:

- Physical USB Host Controller implementations
- USB device firmware
- Custom USB protocol analyzers
- Real-time guarantees for Isochronous transfers

---

# 8. Open Requirements

This section contains requirements that have not yet been fully specified.

(Currently empty.)

---

# 9. Verification

Each requirement shall eventually be verified by one or more of:

- Unit Test
- Integration Test
- Functional Test
- Manual Verification