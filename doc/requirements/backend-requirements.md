# VirtUSB Backend Requirements

**Status:** Draft

# Table of Contents

1. Purpose
2. Scope
3. Definitions and Abbreviations
4. References
5. Backend Overview
6. General Backend Requirements
7. Backend Lifecycle Requirements
8. Virtual Device Requirements
9. USB Transfer Requirements
10. Backend Interface Requirements
11. Backend Quality Requirements
12. Constraints
13. Verification
Appendix A – Requirement Traceability
Appendix B – Reference Documents

# 1. Purpose

This document defines the backend requirements for VirtUSB.

The requirements describe **what** backend implementations shall provide
without specifying **how** these requirements are implemented.

This document refines the Software Requirements by allocating software
responsibilities to backend implementations while remaining independent of
specific implementation details.

This document is primarily intended for developers, software architects, and
reviewers participating in the design, implementation, and evaluation of
VirtUSB backend implementations.

Implementation details are specified in other project documents, including the
Kernel/User Interface Specification, the Userspace Communication Protocol, the
Ownership & Memory Model, the Transfer Queue & Scheduling Model, the
Synchronization & Concurrency Design, and the Architecture Decision Records
(ADRs).

This document intentionally does not specify backend APIs, communication
protocols, algorithms, internal data structures, or other implementation
details.

---

# 2. Scope

This document specifies the requirements applicable to VirtUSB backend
implementations.

The scope includes the functionality required for backend implementations to
provide virtual USB devices and to interact with the VirtUSB userspace
components through the documented backend interface.

This document allocates the software responsibilities defined by the Software
Requirements to backend implementations while remaining independent of their
concrete implementation.

Backend implementations shall remain independent of any specific emulation,
simulation, hardware, or application environment unless explicitly required by
other project documents.

The relationship between backend implementations, the userspace components, and
the kernel module is defined only to the extent necessary to allocate
responsibilities and define backend requirements.

The following are outside the scope of this document:

- backend APIs
- communication protocol definitions
- backend-specific implementation details
- internal algorithms
- internal data structures
- implementation of the kernel module
- implementation of the userspace components

These topics are specified in dedicated project documents.

---

# 3. Definitions and Abbreviations

> **Note:** This section serves as the glossary for this document.

This section defines the terminology and abbreviations used throughout this
document.

The definitions provided here establish a common vocabulary for interpreting
the backend requirements. Where applicable, the terminology is consistent with
the Software Requirements and the High-Level Architecture.

## 3.1 Terminology

| Term | Description |
|------|-------------|
| Backend | Software implementing the behaviour of one or more virtual USB devices |
| Backend Instance | One runtime instance of a backend implementation |
| Virtual USB Device | USB device represented by a backend implementation |
| Endpoint | USB communication endpoint implemented by a virtual USB device |
| USB Transfer | USB transaction processed by a backend implementation |
| Controller | One virtual USB Host Controller instance |
| Controller Interface | Logical interface between one controller instance and its associated userspace component |
| Backend Interface | Logical interface between a backend implementation and the userspace component |
| Backend Capability | Functionality or characteristic provided by a backend implementation and exposed through the documented backend interface |

## 3.2 Abbreviations

The following abbreviations are used throughout this document.

| Abbreviation | Description |
|--------------|-------------|
| USB | Universal Serial Bus |
| API | Application Programming Interface |
| URB | USB Request Block |
| ADR | Architecture Decision Record |

---

# 4. References

The following documents are referenced by this specification.

Unless stated otherwise, the latest approved revision of each project document
applies.

| Reference | Description |
|-----------|-------------|
| High-Level Architecture | Overall architecture of the VirtUSB project |
| System Requirements | System-level functional and non-functional requirements |
| Software Requirements | Software requirements allocated to the VirtUSB software architecture |
| Architecture Decision Records (ADRs) | Documented architectural decisions applicable to the project |
| Kernel/User Interface Specification | Definition of the interface between the kernel module and userspace components |
| Universal Serial Bus Specification, Revision 2.0 | Normative USB specification |
| Linux Kernel Documentation | Linux kernel interfaces, APIs, and development guidelines where applicable |

---

# 5. Backend Overview

Backend implementations provide the software responsible for emulating one or
more virtual USB devices within the VirtUSB architecture.

A backend implements the device-specific behaviour required to process USB
requests, maintain device state, and exchange data with the associated
userspace component through the documented backend interface.

Backend implementations operate independently of the kernel module. The kernel
module is responsible for exposing the virtual USB Host Controller to the
operating system and for forwarding requests between the kernel and the
userspace components.

The userspace components coordinate communication between the kernel module and
the backend implementations. They provide the execution environment in which
backend instances operate and manage their interaction with the controller
interface.

Backend implementations are intentionally independent of specific hardware,
emulation environments, simulation frameworks, or application domains. Any
backend capable of satisfying the requirements defined by this document may be
integrated into the VirtUSB architecture without requiring modifications to
the backend requirements.

---

# 6. General Backend Requirements

Backend implementations shall satisfy the following general requirements:

- provide an abstraction of virtual USB devices independent of the underlying
  implementation technology.
- implement the device behaviour required to satisfy the USB requests forwarded
  through the backend interface.
- remain independent of specific hardware platforms, operating environments,
  application domains, and implementation technologies unless explicitly
  required by other project documents.
- accurately emulate the behaviour of the virtual USB devices they implement,
  including maintaining the associated device state throughout their lifecycle.
- expose their supported capabilities through the documented backend interface.
  Unsupported capabilities shall be reported explicitly.
- manage all allocated resources throughout their lifecycle. Resources shall be
  be released during normal shutdown as well as during error recovery.
- detect internal errors and report them through the documented backend
  interface without compromising the stability of the overall VirtUSB
  architecture.

Backend implementations should also satisfy the following recommendations:

- provide sufficient diagnostic information to support debugging, testing, and
  maintenance.
- ensure that diagnostic output does not alter the externally observable
  behaviour of the backend.

---

# 7. Backend Lifecycle Requirements

Backend implementations shall satisfy the following lifecycle requirements:

- support registration through the documented backend interface before becoming
  available for use.
- perform all required initialization before accepting USB requests.
- enter the active state only after successful initialization.
- stop accepting new USB requests during deactivation while completing or
  terminating outstanding operations in a defined manner.
- perform an orderly shutdown before the backend instance is destroyed.
- release all allocated resources during shutdown regardless of whether the
  shutdown was initiated during normal operation or following an error.
- ensure that failures during lifecycle transitions do not leave the backend in
  an undefined state. Partial initialization or activation shall be rolled
  back or completed in a defined manner.

---

# 8. Virtual Device Requirements

Each backend instance represents exactly one virtual USB device instance.

Backend implementations shall satisfy the following virtual device
requirements:

- represent exactly one virtual USB device instance.
- support creation and removal of the virtual device instance independently of
  its attachment state.
- ensure that a created virtual device instance exists independently of its
  connection to a virtual USB port.
- support attachment of the virtual device instance to a compatible virtual USB
  port.
- support detachment of the virtual device instance without requiring its
  removal.
- ensure that the virtual device instance is attached to at most one virtual
  USB port at any given time.
- maintain all USB-defined device states throughout the lifetime of the virtual
  device instance.
- correctly process USB reset operations in accordance with the USB
  specification.
- provide complete, valid, and internally consistent USB descriptors required
  for successful device enumeration.
- support all USB configurations implemented by the virtual device.
- provide the USB interfaces defined by the active configuration.
- provide the endpoints required by the implemented interfaces.
- ensure that descriptor, configuration, interface, and endpoint information
  remains internally consistent throughout the lifetime of the virtual device
  instance.
- release all resources associated with the virtual device instance when it is
  removed.

---

# 9. USB Transfer Requirements

USB transfer processing shall emulate the behaviour of a physical USB
host controller as observed by the USB host stack.

Backend implementations shall satisfy the following USB transfer
requirements:

-   accept and process all USB transfer types supported by the virtual
    device.
-   preserve the semantics of every submitted USB transfer.
-   preserve transfer data without modification unless explicitly
    required by the USB specification.
-   preserve packet boundaries where required by the USB specification.
-   correctly support short packets and zero-length packets where
    permitted by the USB specification.
-   preserve transfer ordering whenever required by the USB
    specification.
-   ensure that every submitted transfer remains pending until it is
    completed or cancelled.

``` mermaid
stateDiagram-v2
    [*] --> Submitted
    Submitted --> Pending
    Pending --> Completed: successful or failed
    Pending --> Cancelled: cancellation requested
    Completed --> [*]
    Cancelled --> [*]
```

The transfer lifecycle diagram illustrates the externally observable
transfer states. It does not prescribe an internal backend
implementation.

## 9.1 General Transfer Handling

Backend implementations shall:

-   process transfers independently of the transfer type unless USB
    semantics require different behaviour.
-   ensure that transfer processing remains transparent to the USB host
    stack.
-   avoid introducing implementation-specific transfer behaviour visible
    to the host.

## 9.2 Control Transfers

Backend implementations shall:

-   correctly process the Setup, optional Data, and Status stages of
    every control transfer.
-   support Standard, Class, and Vendor requests implemented by the
    virtual device.
-   correctly report unsupported requests in accordance with the USB
    specification.

## 9.3 Bulk Transfers

Backend implementations shall:

-   correctly process bulk transfers in accordance with the USB
    specification.
-   preserve the delivery and error-handling semantics defined for bulk
    endpoints.

## 9.4 Interrupt Transfers

Backend implementations shall:

-   correctly process interrupt transfers in accordance with the USB
    specification.
-   preserve the polling semantics defined by the endpoint descriptor.

## 9.5 Isochronous Transfers

Backend implementations shall:

-   correctly process isochronous transfers in accordance with the USB
    specification.
-   preserve the timing and delivery semantics defined for isochronous
    endpoints.
-   not introduce retransmission behaviour that is not defined for
    isochronous transfers.

## 9.6 Transfer Completion

Backend implementations shall:

-   complete every submitted transfer exactly once.
-   report transfer completion only after transfer processing has
    finished.
-   ensure that transfer completion accurately reflects the observed USB
    transaction outcome.
-   not report a second completion after a transfer has reached a
    terminal state.

## 9.7 Pending Transfers

Backend implementations shall:

-   allow transfers to remain pending until normal completion or
    cancellation.
-   not terminate transfers solely because of implementation-defined
    timeout policies.
-   retain sufficient state to complete or cancel every pending transfer
    deterministically.

## 9.8 Transfer Cancellation

Backend implementations shall:

-   support cancellation of pending transfers.
-   terminate a successfully cancelled transfer with a cancellation
    completion status.
-   ensure that a cancelled transfer cannot later complete with a
    different status.
-   handle races between normal completion and cancellation
    deterministically.

## 9.9 Transfer Status Reporting

Backend implementations shall:

-   report transfer status using USB host-controller semantics.
-   report successful completion, cancellation, and USB-defined transfer
    errors consistently.
-   map backend-internal failures to an appropriate externally
    observable host-controller transfer status.
-   not expose backend implementation details through transfer status
    reporting.

---

# 10. Backend Interface Requirements

The backend interface defines the responsibilities and behavioural
contract between the VirtUSB kernel module and backend implementations.
It intentionally does not define a concrete API or communication
protocol.

Backend interfaces shall satisfy the following requirements:

-   clearly separate backend responsibilities from controller
    responsibilities.
-   remain independent of a specific communication mechanism.
-   provide a stable behavioural contract across compatible interface
    revisions.
-   support capability negotiation.
-   support request handling.
-   support completion handling.
-   support future interface extensions without breaking existing
    implementations.

## 10.1 Interface Responsibilities

Backend interfaces shall:

-   define clear responsibilities for both the VirtUSB kernel module and
    the backend implementation.
-   avoid overlapping responsibilities.
-   avoid requiring backend implementations to replicate controller
    functionality.

## 10.2 Interface Stability

Backend interfaces shall:

-   remain backward compatible within a compatible interface revision.
-   define versioning sufficient to detect incompatible interface
    changes.
-   avoid unnecessary interface changes.

## 10.3 Capability Negotiation

Backend interfaces shall:

-   allow both the VirtUSB kernel module and backend implementations to
    advertise their supported capabilities.
-   distinguish between mandatory and optional capabilities.
-   detect mandatory capability mismatches during interface
    initialization.
-   allow unsupported optional capabilities without preventing
    operation.
-   allow future capabilities to be introduced without changing the
    interface model.

``` mermaid
flowchart LR
    K[Kernel Module] <-- Capability Negotiation --> B[Backend]

    K --> R[Requests]
    B --> C[Completions]
```

The capability negotiation determines the mutually supported interface
features before normal request processing begins.

## 10.4 Request Handling

Backend interfaces shall:

-   define how requests are delivered to backend implementations.
-   preserve request ordering where required.
-   avoid exposing implementation-specific transport behaviour.

## 10.5 Completion Handling

Backend interfaces shall:

-   define how completed requests are reported.
-   guarantee that every completed request is reported exactly once.
-   preserve completion ordering where required.

## 10.6 Future Extensibility

Backend interfaces shall:

-   support addition of new optional interface features.
-   preserve compatibility for existing implementations whenever
    optional features are introduced.
-   allow interface evolution without changing the architectural model.

--- 

# 11. Backend Quality Requirements

Backend implementations shall satisfy the following quality requirements
to ensure long-term maintainability, reliable operation, and efficient
resource usage across supported platforms.

## 11.1 Maintainability

Backend implementations shall:

-   use a clear and consistent software architecture.
-   separate functional components with well-defined responsibilities.
-   minimise unnecessary implementation complexity.
-   document externally observable behaviour where appropriate.

## 11.2 Reliability

Backend implementations shall:

-   operate correctly during continuous operation.
-   maintain a consistent internal state throughout their lifetime.
-   recover gracefully from recoverable error conditions.
-   avoid data corruption and resource leaks.

## 11.3 Robustness

Backend implementations shall:

-   validate externally supplied input before processing.
-   tolerate invalid or unexpected requests without undefined behaviour.
-   fail predictably and consistently when unrecoverable errors occur.
-   preserve interface integrity during error handling.

## 11.4 Testability

Backend implementations shall:

-   support verification of externally observable behaviour.
-   permit automated testing where practical.
-   produce deterministic behaviour for identical inputs and operating
    conditions.
-   allow error conditions to be reproduced during testing.

``` mermaid
flowchart LR
    Tests --> Backend
    Backend --> Results
    Results --> Verification
```

The quality requirements shall enable automated verification of backend
behaviour independently of a specific backend implementation.

## 11.5 Portability

Backend implementations shall:

-   avoid unnecessary dependencies on a specific operating system or
    runtime.
-   isolate platform-specific functionality where required.
-   preserve consistent externally observable behaviour across supported
    platforms.

## 11.6 Resource Efficiency

Backend implementations shall:

-   use CPU time, memory, and other system resources efficiently.
-   release resources promptly when no longer required.
-   avoid unnecessary resource consumption during idle operation.
-   scale resource usage with the workload whenever practical.

---

# 12. Constraints

Backend implementations shall comply with the architectural and
development constraints defined by the VirtUSB project. These
constraints ensure that independent backend implementations remain
compatible with the documented architecture, interface model, and
long-term project objectives.

``` mermaid
flowchart TD
    Architecture["Project Architecture"] --> Backend["Backend Implementation"]
    Requirements["Requirements"] --> Backend
    Interface["Backend Interface Specification"] --> Backend
    Conventions["Implementation & Documentation Conventions"] --> Backend
    Dependencies["External Dependencies"] --> Backend
```

The diagram illustrates that backend implementations are governed by
multiple project documents and constraints rather than by the backend
interface alone.

## 12.1 Compliance with Project Architecture

Backend implementations shall:

-   comply with the documented VirtUSB architecture.
-   preserve the architectural separation of responsibilities between
    the controller and the backend.
-   not introduce behaviour that conflicts with documented architectural
    decisions.

## 12.2 Compliance with Backend Interface Specification

Backend implementations shall:

-   comply with the documented backend interface specification.
-   implement all mandatory interface requirements.
-   correctly identify interface incompatibilities before normal
    operation begins.

## 12.3 Implementation Conventions

Backend implementations shall:

-   follow the project's documented implementation conventions.
-   use consistent naming, coding, and formatting conventions.
-   avoid implementation practices that reduce maintainability or
    portability.

## 12.4 Documentation Conventions

Backend implementations shall:

-   follow the project's documentation conventions.
-   document externally observable behaviour where appropriate.
-   keep implementation documentation consistent with the implemented
    functionality.

## 12.5 External Dependencies

Backend implementations shall:

-   minimise unnecessary external dependencies.
-   document all required external dependencies.
-   avoid dependencies that unnecessarily restrict portability.
-   ensure that external dependencies remain compatible with the
    project's licensing and distribution requirements.

---

# 13. Verification

Compliance with this specification shall be verified using documented
and repeatable verification activities. The selected verification
methods shall provide objective evidence that backend implementations
satisfy the specified requirements.

``` mermaid
flowchart LR
    Requirements --> Traceability
    Traceability --> Reviews
    Reviews --> StaticAnalysis
    StaticAnalysis --> UnitTests
    UnitTests --> IntegrationTests
    IntegrationTests --> CI["Continuous Integration"]
```

The verification activities complement each other and collectively
provide evidence that a backend implementation conforms to the
documented requirements.

## 13.1 Requirement Traceability

Verification activities shall:

-   establish traceability between requirements and verification
    evidence.
-   ensure that every mandatory requirement is covered by at least one
    verification activity.
-   document any justified deviations.

## 13.2 Documentation Reviews

Project documentation shall:

-   be reviewed for technical correctness and consistency.
-   remain consistent across related project documents.
-   be updated when implementation or architectural changes affect
    documented behaviour.

## 13.3 Static Code Analysis

Backend implementations shall:

-   be verified using one or more static code analysis tools.
-   resolve or justify reported findings before release.
-   perform static analysis as part of the normal development process.

## 13.4 Unit Testing

Backend implementations shall:

-   verify individual functional components using unit tests where
    practical.
-   verify normal operation and relevant error conditions.
-   produce repeatable test results.

## 13.5 Integration Testing

Backend implementations shall:

-   verify correct interaction with the VirtUSB kernel module.
-   verify supported USB transfer types and device lifecycle operations.
-   verify externally observable behaviour against the documented
    requirements.

## 13.6 Continuous Integration

The verification process shall:

-   support automated execution of verification activities where
    practical.
-   execute static analysis and automated tests within the continuous
    integration workflow.
-   prevent verification regressions from remaining undetected.
-   archive verification results for later review where practical.

---

# Appendix A – Requirement Traceability (Example)

The following table illustrates the recommended structure of a requirement
traceability matrix. The actual project traceability matrix is maintained
separately.

| Requirement | Verification | ADR | Status |
|-------------|--------------|-----|--------|
| ...         | ...          | ... | ...    |

---

# Appendix B – Reference Documents (Example)

The following table illustrates the recommended structure for maintaining
project reference documents.

| ID      | Document | Version |
|---------|----------|---------|
| ...     | ...      | ...     |
