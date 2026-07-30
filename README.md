# VirtUSB

> A virtual USB Host Controller for Linux.

VirtUSB is an open-source project that aims to provide a virtual USB Host Controller
for Linux. It enables the development, testing and simulation of USB devices without
requiring dedicated USB hardware or a microcontroller for many development tasks.

The project focuses on emulating the host controller rather than implementing another
USB stack or a generic USB emulator. Virtual USB devices can be attached to the
controller and are enumerated by the Linux USB subsystem like physical devices.

## Project Goals

- Develop a virtual USB Host Controller for Linux.
- Support multiple virtual host controllers.
- Support all major USB transfer types:
  - Control
  - Bulk
  - Interrupt
  - Isochronous
- Allow backend-independent USB device implementations.
- Make virtual devices visible through the normal Linux USB subsystem (e.g. `lsusb`).
- Provide a foundation for implementing USB devices that conform to the USB 2.0
  specification, Chapter 9 (USB Device Framework).

## Current Status

VirtUSB is currently in the architecture and design phase.

No stable API exists yet and the project structure may change as architectural
decisions are made.

## Documentation

Project documentation is located in the `doc/` directory.

Architecture decisions are documented using the ADR (Architecture Decision Record)
format.

## Development

Current development targets include:

- Linux (primary platform)
- GCC and LLVM/Clang
- CMake build system
- Doxygen documentation
- cppcheck and clang-tidy for static analysis
- DKMS support for the kernel module

## Contributing

Contributions are welcome.

Please read the documentation in the `doc/` directory before contributing.
Additional documents such as the Contribution Guide, Code of Conduct and
Architecture Documentation will be added as the project evolves.

## License

The final project license has not yet been selected.

The licensing goals and philosophy will be documented before a final license is
chosen.

## Disclaimer

This project is **not** related to the historical Windows project **USB-VHCI**
or its component named **virtusb**.
