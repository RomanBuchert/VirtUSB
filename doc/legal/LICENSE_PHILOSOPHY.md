# License Philosophy

> **Status:** Draft
>
> This document describes the licensing goals of the VirtUSB project.
> It does **not** define the final software license.

## Introduction

Choosing a software license is an architectural decision as much as a legal one.

The goal is not simply to select an existing license, but to choose one that
supports the long-term objectives of the project.

The final license will be selected only after the overall architecture and
extension mechanisms have been defined.

## Contribution Policy During Development

> **Important**
>
> The following policy applies until the project's final license has been
> officially announced.

The final license of VirtUSB has not yet been selected.

During the architecture and early development phases, all contributions are
accepted under the explicit understanding that the project maintainers may
change the project's license before the final project license has been
officially announced if doing so is necessary to achieve the project's
licensing goals described in this document.

By contributing code, documentation or other project material, contributors
agree that their contributions may be redistributed under the project's future
official license.

Contributors who are not willing to grant these rights must not submit
contributions before the licensing process has been completed.

Until the project's final license has been officially announced, the source
code, documentation and all other project material may **not** be incorporated
into proprietary or closed-source products or distributed as part of such
products without explicit written permission from the project maintainers.

This temporary restriction exists solely to protect the project's future
licensing options. It is **not** intended to make VirtUSB proprietary.

The intention is to keep the choice between suitable open-source licenses
(for example GPL, LGPL, MPL, MIT or similar licenses) open until the
architecture and public interfaces have matured.

## Project Goals

The licensing model should satisfy the following objectives:

- Keep the VirtUSB core open.
- Encourage improvements to be contributed back to the project.
- Allow third parties to develop independent backends.
- Prevent proprietary extensions from creating a hidden or incompatible API.
- Keep the public API stable and well documented.
- Support both open-source and commercial use where appropriate.

These goals may compete with each other and therefore require careful
consideration.

## Kernel Module

VirtUSB contains a Linux kernel module.

The Linux kernel licensing model places additional requirements on kernel
modules. The impact of these requirements on the overall licensing strategy
must be evaluated before a final decision is made.

## Backends

One of the architectural goals of VirtUSB is backend neutrality.

Different backend implementations may exist for different use cases and
programming languages.

Whether all backends must be open source, or whether proprietary backends are
acceptable, has not yet been decided.

## Public API

The public API is one of the project's most valuable interfaces.

The licensing model should encourage extensions to use the documented public
API instead of introducing incompatible private interfaces.

The project should avoid a situation where proprietary extensions depend on
undocumented kernel interfaces while the public API becomes a second-class
interface.

## Open Questions

The following questions remain open:

- Which license best matches the project goals?
- Should all API extensions be contributed back?
- Which project components should share the same license?
- Can proprietary backends coexist with an open VirtUSB core?
- How can "shadow APIs" be discouraged without unnecessarily restricting
  legitimate use cases?

These questions will be answered before selecting the final project license.

## Final Decision

The final project license will be documented separately once these questions
have been resolved.
