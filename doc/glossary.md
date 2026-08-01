# VirtUSB Glossary

**Status:** Draft

# 1. Purpose

This document defines the common terminology and abbreviations used throughout the
VirtUSB documentation.

It serves as the authoritative reference for project-specific terms. Individual
documents should reference this glossary instead of redefining common terminology.
Document-specific terms may be introduced locally where required.

---

# 2. Terminology

The following definitions apply throughout the VirtUSB project unless explicitly
specified otherwise.

| Term | Definition |
|------|------------|
| Associated | State in which a backend is logically assigned to exactly one parent port. |
| Association | Logical assignment of a backend to exactly one parent port within the virtual USB topology. |
| Attachment | Topological operation attaching a virtual USB device to a parent port. Attachment does not imply USB connection or successful enumeration. |
| Backend Authority | Scope of operations a backend is permitted to perform. A backend may control only the virtual hardware of the device it represents. |
| Connection | Host-visible USB connection state of a virtual USB device. |
| Controller Topology | Hierarchical organisation of the virtual USB tree consisting of the Root Hub, optional USB hubs and their ports. |
| Device Hardware State | Operational state of the virtual device hardware independent of USB protocol state. |
| Device Power State | Power state of the virtual device hardware. |
| Parent Hub | USB hub owning a parent port. The Root Hub is the top-most parent hub. |
| Parent Port | Port to which a virtual USB device is topologically attached. |
| Topology | Hierarchical arrangement of virtual USB devices and hubs. |
| USB Hardware Availability | State indicating whether the virtual USB device controller is able to participate in USB communication. |
| USB Protocol Operation | USB-defined behaviour including enumeration, standard requests, class requests and data transfers. |
| Virtual Device Hardware | Virtual representation of the hardware of one emulated USB device. |
| Virtual Host Controller and Topology | VirtUSB control plane responsible for controller instances, hubs, ports and device topology. |
| Responsibility Domain | Architectural view describing ownership, state boundaries and behavioural responsibilities independently of software implementation. |
| Runtime Object | Architectural object that exists during system operation, has an independent lifecycle and explicit ownership. |
| Software Component | Architectural element describing where functionality is implemented. Software components define implementation boundaries rather than ownership or runtime behaviour. |
| Backend | Userspace software component representing exactly one virtual device hardware instance and implementing its device-specific USB behaviour. |
| Backend Instance | Runtime instance of a backend representing exactly one virtual device hardware instance. |
| Backend Lifecycle | Lifetime of a backend instance from creation until destruction. |
| Connected | State in which a virtual USB device is visible to the host operating system. |
| Connection | Host-visible USB connection between attached virtual device hardware and the Linux USB subsystem. |
| Controller | Runtime instance of a virtual USB Host Controller. |
| Controller Instance | One independent virtual USB Host Controller including its Root Hub, ports and controller interface. |
| Controller Interface | Logical interface between one controller instance and userspace. |
| Controller Lifecycle | Lifetime of a controller instance from creation until removal. |
| Device | Runtime representation of one virtual USB device. |
| Device Attachment | Operation attaching a virtual USB device to its assigned parent port within the virtual USB topology. |
| Device Detachment | Operation removing a virtual USB device from its parent port. |
| Disconnected | State in which a virtual USB device is not visible to the host operating system. |
| Root Hub Port | Parent port belonging to the Root Hub. |
| Endpoint | Communication endpoint of a virtual USB device. |
| Enumerated | State in which the host has successfully completed USB enumeration of a virtual USB device. |
| Enumeration | Standard USB process performed by the Linux USB subsystem after a connected device becomes visible on the USB bus. |
| Host Controller | Virtual USB Host Controller exposed to the Linux USB subsystem. |
| Ownership | Exclusive responsibility for the lifetime and cleanup of a resource. |
| Port | Runtime object belonging to exactly one hub and capable of hosting at most one associated virtual USB device. |
| Root Hub | Top-level virtual USB hub owned by one controller instance. |
| Runtime Object | Architectural object with an independent lifetime, such as a controller, port, backend or transfer. |
| Transfer | Runtime representation of one USB transfer from submission until completion or cancellation. |
| Unassociated | State in which a backend is not assigned to any downstream port. |
| Virtual USB Device | Emulated USB device represented by exactly one backend instance. Visibility to the host depends on topology and USB connection state. |

---

# 3. Abbreviations

| Abbreviation | Meaning |
|--------------|---------|
| ADR | Architecture Decision Record |
| API | Application Programming Interface |
| DKMS | Dynamic Kernel Module Support |
| EP | Endpoint |
| HCD | Host Controller Driver |
| HCI | Host Controller Interface |
| IPC | Inter-Process Communication |
| SOF | Start of Frame |
| URB | USB Request Block |
| USB | Universal Serial Bus |

---

# 4. Maintenance

New project-wide terminology shall be added to this document.

Existing definitions shall not be redefined in individual project documents.
Document-specific terminology may be introduced only where necessary and shall
not conflict with the definitions provided by this glossary.
