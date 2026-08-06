# System Overview

VirtUSB is a project that provides virtual USB Host Controllers
(`VirtUsbHcd`) to which virtual USB devices (`VirtUsbDev`) can be
attached through a virtual USB Root Hub (`VirtUsbRHub`).

**VirtUSB virtualizes USB devices to enable their software to be
developed, tested, and debugged independently of the target hardware.**

VirtUSB exclusively supports USB 2.0.

VirtUSB is designed for Linux as the host operating system.

VirtUSB does not emulate physical USB hardware. In particular, it does
not emulate the electrical signaling, bit level, or packet level of a
USB bus. Communication takes place at a higher level of USB abstraction.

Each **core component** exposes a **well-defined interface** for
controlling its functionality and state.

Together, the core components form a fully virtual USB infrastructure.
Only virtual USB devices can be attached to a `VirtUsbHcd`. Mixing
virtual and physical USB components within the same VirtUSB topology is
not intended.

The meaning of the terminology used throughout the project is defined in
the Glossary.
