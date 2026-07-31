# VirtUSB Software Requirements

**Status:** Draft

# Table of Contents

1. Purpose
2. Scope
3. Definitions and Abbreviations
4. References
5. Software Overview
6. Kernel Module Requirements
7. Userspace Requirements
8. Backend Integration Requirements
9. Internal Interface Requirements
10. Software Quality Requirements
11. Constraints
12. Verification
Appendix A – Requirement Traceability
Appendix B – Reference Documents

# 1. Purpose

This document defines the software requirements for VirtUSB.

The requirements describe **what** the VirtUSB software components shall provide
without specifying **how** these requirements are implemented.

This document refines the System Requirements by allocating responsibilities to
the individual software components while remaining independent of specific
implementation details.

This document is primarily intended for developers, software architects, and
reviewers participating in the design and implementation of VirtUSB.

Implementation details are specified in other project documents, including the
Kernel/User Interface Specification, the Userspace Communication Protocol, the
Ownership & Memory Model, the Transfer Queue & Scheduling Model, the
Synchronization & Concurrency Design, and the Architecture Decision Records
(ADRs).

This document intentionally does not specify algorithms, data structures,
internal interfaces, communication protocols, or other implementation details.

---

# 2. Scope

This document specifies the software requirements for the VirtUSB software
system.

The scope includes the software components required to implement the system
requirements defined in the VirtUSB System Requirements specification.

The software components covered by this document include:

- Linux kernel module
- userspace components
- backend integration
- software interfaces between these components

This document allocates responsibilities to the individual software
components while remaining independent of their concrete implementation.

The software architecture described in the High-Level Architecture provides the
overall structural framework for these requirements.

The following are outside the scope of this document:

- implementation details
- internal algorithms
- communication protocol definitions
- internal data structures
- synchronization mechanisms
- ownership and memory management
- backend-specific implementations

These topics are specified in dedicated project documents.

---

# 3. Definitions and Abbreviations

> **Note:** This section serves as the glossary for this document.

This section defines the terminology and abbreviations used throughout this
document.

The definitions provided here establish a common vocabulary for interpreting
the software requirements. Where applicable, the terminology is consistent with
the System Requirements and the High-Level Architecture.

## 3.1 Terminology

| Term | Description |
|------|-------------|
| Kernel Module | Linux kernel module implementing the VirtUSB Host Controller functionality |
| Userspace Component | Software communicating with the kernel module through the documented interface |
| Backend | Software implementing the behaviour of a virtual USB device |
| Controller | One virtual USB Host Controller instance |
| Root Hub | Virtual USB Root Hub owned by one controller |
| Port | One downstream Root Hub port |
| Device | Virtual USB device represented by a backend |
| Controller Interface | Logical interface between one controller and its associated userspace component |

## 3.2 Abbreviations

The following abbreviations are used throughout this document.

| Abbreviation | Description |
|--------------|-------------|
| USB | Universal Serial Bus |
| HCD | Host Controller Driver |
| HCI | Host Controller Interface |
| URB | USB Request Block |
| API | Application Programming Interface |
| ADR | Architecture Decision Record |

---

# 4. References

This document is based on the following references.

Project-specific documents define the VirtUSB software architecture and its
requirements. External references provide the technical background relevant to
this specification.

- VirtUSB High-Level Architecture
- VirtUSB System Requirements
- Architecture Decision Records (ADRs)
- Universal Serial Bus Specification, Revision 2.0
- Linux Kernel Documentation
- Linux USB Subsystem Documentation

---

# 5. Software Overview

This section provides an overview of the software structure of VirtUSB.

The software is decomposed into logical components, each having clearly defined
responsibilities and interfaces.

The decomposition described in this document is derived from the High-Level
Architecture and allocates the system requirements to the corresponding
software components.

The major software components include:

- Linux kernel module
- userspace components
- backend implementations

Each software component shall have clearly defined responsibilities and shall
interact with other components only through documented interfaces.

Responsibility boundaries shall minimize coupling between components and
support independent implementation, testing, and maintenance.

The interaction between software components shall be consistent with the
overall system architecture defined by the High-Level Architecture.

---

# 6. Kernel Module Requirements

The VirtUSB kernel module shall provide the functionality required to implement
one or more independent virtual USB Host Controller instances.

Its responsibilities include:

- creation, initialization, operation, and removal of controller instances
- creation and management of one virtual Root Hub for each controller
- management of Root Hub port state and status changes
- representation, attachment, disconnection, and removal of virtual USB devices
- submission, forwarding, completion, cancellation, and cleanup of USB transfers
- support for Control, Bulk, Interrupt, and Isochronous transfer types
- notification of relevant controller, port, device, and transfer events
- allocation, tracking, and release of kernel resources
- management of object lifetimes across asynchronous operations
- handling of invalid requests, backend failures, disconnects, and shutdown conditions
- provision of logging and diagnostic information

Independent controller instances shall operate independently and shall not
interfere with each other.

All resources allocated by the kernel module shall be released when a controller
instance is removed or when the module is unloaded.

---

# 7. Userspace Requirements

The VirtUSB userspace component shall provide the functionality required to
control virtual USB Host Controller instances and to interface backend
implementations with the kernel module.

Its responsibilities include:

- connection to and lifecycle management of controller instances
- registration and management of backend implementations
- representation, attachment, disconnection, and removal of virtual USB devices
- processing of controller, port, device, and transfer events received from the kernel module
- submission of backend requests and transfer responses to the kernel module
- allocation, tracking, and release of userspace resources
- handling of communication failures, backend failures, invalid requests, and controller shutdown conditions
- provision of logging and diagnostic information

The userspace component shall communicate with the kernel module exclusively
through the documented controller interface.

The userspace component shall isolate independent controller instances to
prevent unintended interactions between them.

The userspace component shall release all allocated resources when a controller
connection is closed or when the application terminates.

---

# 8. Backend Integration Requirements

Backend integration shall satisfy the following requirements:

- remain independent of any specific backend implementation
- support integration through the documented backend interface without requiring
  modifications to the kernel module
- support the discovery and association of backend implementations with
  controller instances and virtual USB devices
- support a well-defined backend lifecycle, including registration,
  activation, deactivation, and removal
- provide a mechanism to discover backend capabilities through the documented
  backend interface
- communicate exclusively through the documented backend interface
- detect and handle communication failures, invalid operations, and backend
  failures in a controlled manner

Backend implementations shall not require modifications to the VirtUSB kernel
module in order to support backend-specific functionality.

The backend interface shall remain sufficiently generic to support different
backend architectures and implementation strategies.

---

# 9. Internal Interface Requirements

Internal software interfaces shall satisfy the following requirements:

- provide clearly defined responsibilities for each software component
- separate software components through documented interfaces
- expose only the functionality required for component interaction
- use consistent interface design principles throughout the project
- support future extensions while preserving compatibility where practical
- remain stable once publicly documented

Internal interfaces shall minimize coupling between software components and
support independent implementation, testing, and maintenance.

The interaction between the kernel module and userspace components shall be
performed exclusively through the documented controller interface.

Detailed interface specifications are provided in the dedicated interface
documentation.

---

# 10. Software Quality Requirements

The VirtUSB software shall satisfy the following quality requirements:

- be maintainable
- support unit testing and integration testing of individual software
  components
- provide reliable operation under both normal and abnormal conditions
- support future extensions without requiring unnecessary architectural
  changes
- remain portable across supported compiler toolchains and Linux
  environments
- use system resources efficiently while maintaining correctness,
  readability, and maintainability
- provide appropriate protection against invalid input and misuse

The software quality requirements defined in this document shall apply to all
software components unless explicitly stated otherwise.

---

# 11. Constraints

The VirtUSB software shall comply with the following constraints:

- follow the architectural principles defined by the High-Level Architecture
- target the supported Linux platforms and compiler toolchains defined by the project
- use the project's documented build system and development toolchain
- comply with the project's coding and documentation conventions
- minimize unnecessary external dependencies

Userspace components shall use Doxygen for API documentation.

Kernel components shall follow the Linux kernel coding style and documentation
conventions. Kernel interfaces and relevant internal kernel functions shall use
kernel-doc where appropriate.

Detailed coding and documentation conventions, including Doxygen and
kernel-doc examples, are provided in `code-documentation-examples.md`.

All external dependencies shall be selected to minimize complexity while
remaining suitable for the project requirements.

---

# 12. Verification

Compliance with the software requirements defined in this document shall be
verified using appropriate verification methods.

Verification activities include:

- maintenance of requirement traceability
- documentation reviews
- static code analysis
- unit testing
- integration testing
- continuous integration

The selected verification method shall be appropriate for the requirement being
verified.

Requirement traceability shall demonstrate the relationship between
requirements, implementation, and verification activities.

Documentation reviews shall verify the technical correctness, completeness, and
consistency of project documentation.

Static code analysis shall be used to identify potential defects and violations
of the project's coding conventions.

Unit testing shall verify the behaviour of individual software components.

Integration testing shall verify the interaction between software components and
the correct operation of the complete software system.

Continuous integration shall support automated build, analysis, and testing
activities where practical.

---

# Appendix A – Requirement Traceability (Example)

The following table illustrates the recommended structure of the requirement
traceability matrix. The actual project traceability matrix is maintained
separately.

| Requirement | Verification | ADR | Status |
|-------------|--------------|-----|--------|
| VUSB-SWR-001 | ... | ... | Draft |
...

---

# Appendix B – Reference Documents (Example)

The following table illustrates the recommended structure for maintaining
project references.

| ID | Document | Version |
|----|----------|---------|
| REF-001 | High-Level Architecture | v0.0.1 |
...
