# Contributing to VirtUSB

Thank you for your interest in contributing to VirtUSB.

VirtUSB is an open-source project that aims to provide a virtual USB Host Controller
for Linux. Contributions of all kinds are welcome, including code, documentation,
bug reports, feature requests and design discussions.

## Communication

The project language is **English**.

Many contributors are not native English speakers. Please focus on the technical
content of a discussion rather than the wording. If something appears overly direct,
assume good intentions before responding.

Personal attacks, insults and disrespectful behaviour are not tolerated. Please
keep discussions constructive and professional.

## Before You Start

Before implementing larger changes, please discuss your idea first.

The project intentionally follows an architecture-first approach. Significant
architectural changes should be discussed before code is written.

## Coding Guidelines

General requirements:

- Write portable and maintainable C code.
- Keep implementations as simple as possible.
- Avoid unnecessary complexity.
- Prefer readable code over clever code.
- Keep functions focused on a single responsibility.

Additional project-specific rules are documented separately.

## Documentation

Documentation is written in Markdown.

Architecture decisions are documented using the ADR (Architecture Decision Record)
format.

Public APIs should be documented with Doxygen.

## Development Environment

Preferred development environment:

- Linux (Debian or Arch Linux)
- GCC and LLVM/Clang
- CMake
- Doxygen
- cppcheck
- clang-tidy

The kernel module should remain compatible with DKMS.

## Pull Requests

Please ensure that:

- the code builds successfully,
- static analysis passes,
- documentation is updated where necessary,
- unrelated changes are not mixed into the same pull request.

Small, focused pull requests are preferred over large ones.

## Artificial Intelligence

The use of AI tools is explicitly permitted.

AI should be treated as a development tool similar to a compiler, debugger or
static analysis tool. Contributors remain responsible for understanding,
reviewing and validating every submitted change.

## Reporting Issues

Bug reports should include, whenever possible:

- Linux distribution and kernel version
- compiler and compiler version
- build configuration
- steps to reproduce the issue
- expected behaviour
- actual behaviour

## Code of Conduct

By participating in this project you agree to follow the project's
Code of Conduct.
