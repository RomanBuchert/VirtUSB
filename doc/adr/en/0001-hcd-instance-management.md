# ADR-0001: HCD Instance Management

-   **Status:** Proposed
-   **Date:** 2026-08-06

------------------------------------------------------------------------

# Context

VirtUSB supports multiple independent virtual USB host controllers
(`VirtUsbHcd`) within a single kernel module.

The High-Level Architecture already establishes that multiple
`VirtUsbHcd` instances may exist simultaneously, with each instance
forming the root of its own VirtUSB topology.

The implementation must define:

-   how many HCD instances are created,
-   how their lifecycle is managed,
-   how the instances are stored,
-   and how errors during initialization are handled.

This decision affects the lifecycle of the kernel module as a whole and
would require substantial refactoring to change later.

------------------------------------------------------------------------

# Decision

The number of `VirtUsbHcd` instances to be created is specified when the
kernel module is loaded by means of a module parameter.

The module parameter has:

-   default value: `1`
-   valid range: `1..31`

The upper limit of 31 is a project-specific safety limit and does not
represent a limitation imposed by the USB specification.

The module parameter cannot be changed after the kernel module has been
loaded.

Module-wide state whose size depends on the number of HCD instances is
allocated dynamically.

A separate Linux backing object is created for each HCD instance.

Each HCD instance maintains its own private state.

Initialization follows an all-or-nothing policy.

If any HCD instance cannot be created successfully, all previously
created instances are removed in reverse order and initialization of the
kernel module fails as a whole.

When the kernel module is unloaded, all HCD instances are removed in a
controlled manner.

------------------------------------------------------------------------

# Considered Alternatives

## Single Global HCD Instance

Exactly one global `VirtUsbHcd` instance exists.

### Assessment

Rejected.

This approach conflicts with the established architecture, which allows
multiple independent VirtUSB topologies to exist simultaneously.

------------------------------------------------------------------------

## Static Array with up to 31 Instances

A static array manages all possible HCD instances.

### Assessment

Technically feasible.

This approach is simple to implement, but reserves memory independently
of the number of instances actually used and represents the set of
instantiated HCDs less precisely.

------------------------------------------------------------------------

## Dynamic Management Based on the Requested Instance Count

Module-wide state is allocated dynamically according to the actual
number of HCD instances requested.

### Assessment

Selected.

This approach allocates only the memory actually required, directly
represents the requested configuration, and simplifies error handling
and HCD instance lifecycle management.

------------------------------------------------------------------------

## Dynamic Creation and Removal at Runtime

HCD instances can be created or removed arbitrarily after the kernel
module has been loaded.

### Assessment

Rejected.

VirtUSB currently has no functional requirement for dynamically changing
the HCD configuration at runtime.

Such an approach would unnecessarily complicate lifecycle management,
synchronization, and the associated interfaces.

------------------------------------------------------------------------

# Consequences

## Advantages

-   Supports multiple independent VirtUSB topologies.
-   Low memory overhead through dynamic allocation.
-   Clearly defined lifecycle for each HCD instance.
-   Fully deterministic initialization.
-   Unambiguous error handling.
-   Good extensibility for future implementation steps.

## Disadvantages

-   More complex initialization than a single-instance design.
-   Rollback logic is required when initialization fails.

------------------------------------------------------------------------

# Implementation

This decision is implemented primarily in the following components:

-   `virtusb_module.c`
-   `virtusb_hcd.c`

It forms the basis for managing all `VirtUsbHcd` instances.
