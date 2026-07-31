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
| Associated | State in which a backend is logically assigned to exactly one downstream port. |
| Association | Logical assignment of a backend to a virtual downstream port. |
| Attachment | Operation making an associated virtual USB device visible on a virtual downstream port. |
| Backend | Userspace component implementing the behaviour of a virtual USB device. |
| Backend Instance | One runtime instance of a backend representing exactly one virtual USB device instance. |
| Backend Lifecycle | Lifetime of a backend instance from creation until destruction. |
| Connected | State in which a virtual USB device is visible to the host operating system. |
| Connection | Host-visible presence of a virtual USB device on a virtual downstream port. |
| Controller | Runtime instance of a virtual USB Host Controller. |
| Controller Instance | One independent virtual USB Host Controller including its Root Hub, ports and controller interface. |
| Controller Interface | Logical interface between one controller instance and userspace. |
| Controller Lifecycle | Lifetime of a controller instance from creation until removal. |
| Device | Runtime representation of one virtual USB device. |
| Device Attachment | Operation connecting an associated virtual USB device to its assigned downstream port. |
| Device Detachment | Operation disconnecting a virtual USB device while preserving its backend association. |
| Disconnected | State in which a virtual USB device is not visible to the host operating system. |
| Downstream Port | Attachment point for at most one virtual USB device. |
| Endpoint | Communication endpoint of a virtual USB device. |
| Enumerated | State in which the host has successfully completed USB enumeration of a virtual USB device. |
| Host Controller | Virtual USB Host Controller exposed to the Linux USB subsystem. |
| Ownership | Exclusive responsibility for the lifetime and cleanup of a resource. |
| Port | Runtime object belonging to exactly one Root Hub and capable of hosting at most one associated backend. |
| Root Hub | Virtual upstream USB hub owned by one controller instance. |
| Runtime Object | Architectural object with an independent lifetime, such as a controller, port, backend or transfer. |
| Transfer | Runtime representation of one USB transfer from submission until completion or cancellation. |
| Unassociated | State in which a backend is not assigned to any downstream port. |
| Virtual USB Device | USB device visible to the host while connected through VirtUSB. |

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
