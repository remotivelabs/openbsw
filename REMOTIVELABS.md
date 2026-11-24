<!--
 *******************************************************************************
  Copyright (c) 2026 Remotive Labs AB

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
 *******************************************************************************
-->

# Remotive Labs fork of Eclipse OpenBSW

This is [Remotive Labs](https://remotivelabs.com)' fork of [Eclipse OpenBSW](https://github.com/eclipse-openbsw/openbsw).
It carries the changes that the RemotiveCar `BCM` needs. That BCM is an OpenBSW `referenceApp` application, which runs
as a virtual ECU in a container and on an NXP S32K148EVB board. Its sources, Dockerfile and documentation are in
[remotivelabs-remotive-car](https://github.com/remotivelabs/remotivelabs-remotive-car) under `models/bcm/openbsw`.
Builds use tags of the `remotive/main` branch, `remotive/<date>`.

## Branches

Each change is on its own topic branch, which starts from the upstream commit that `remotive/main` is based on. Each
topic branch can be offered upstream on its own. `remotive/main` merges all of them, and adds this file.

| Branch | What it changes | Meant for upstream |
|---|---|---|
| `fix/idle-platform-rest` | The referenceApp idle handler calls a `platformIdle()` callout, and the POSIX platform sleeps one tick in it. Idle CPU use on a Linux host drops from ~92 % of a core to ~14 %. | yes |
| `fix/uds-ecu-reset` | A UDS ECU Reset (0x11) now resets the ECU (process exit on POSIX, MCU reset on hardware). Before, it was acknowledged and then left diagnostics disabled. | yes |
| `fix/posix-aarch64-stack-min` | Uses the 128 KiB AArch64 `PTHREAD_STACK_MIN` on POSIX, so tasks get the stack they are configured with on arm64 hosts. | yes |
| `fix/s32k148-unsecured` | Writes FSEC = 0xFE to the S32K148 flash configuration field, so the MCU is not secured (and locked out from debuggers) after the first flash. | yes |
| `feat/second-can-bus` | Adds `busid::CAN_1` and backs it with `vcan1` on POSIX. | yes |
| `feat/s32k148-flexcan1` | Backs `CAN_1` with FlexCAN1 on the S32K148EVB (PTA13/PTA12, J1-23/J5-8). Builds on `feat/second-can-bus`. | yes |
| `feat/posix-can-config-from-env` | The POSIX CAN interfaces and frame format come from `OPENBSW_CAN0_IF`, `OPENBSW_CAN1_IF` and `OPENBSW_CAN_FD`, so one binary runs on any interfaces. Builds on `feat/second-can-bus`. | yes |
| `feat/sw-version` | The UDS software version (DID F195) is set by the build with `-DREFERENCE_APP_SW_VERSION=...`. | yes |
| `feat/overridable-platform-options` | The platform `Options.cmake` files no longer `FORCE` the feature options, so `-DPLATFORM_SUPPORT_...=OFF` works. | yes |
| `feat/external-application` | `-DREFERENCE_APP_APPLICATION_DIR=<dir>` builds an application from outside the tree in place of the referenceApp demo. | yes |
| `remotive/local` | Changes specific to the RemotiveCar BCM: log levels, the application name `BCM` and its logger component, and 8 CAN frames per run on POSIX. | no |

Every topic branch builds the unmodified referenceApp. The commit messages explain each change.

## Building the RemotiveCar BCM

The BCM is not in this repository. Its Dockerfile clones a `remotive/<date>` tag and builds the referenceApp with the
BCM as an external application. For POSIX, it builds roughly this:

```bash
cmake --preset posix-freertos \
    -DPLATFORM_SUPPORT_ETHERNET=OFF -DPLATFORM_SUPPORT_STORAGE=OFF -DPLATFORM_SUPPORT_MIDDLEWARE=OFF \
    -DPLATFORM_SUPPORT_UDS_DEMO_SERVICES=ON \
    -DREFERENCE_APP_APPLICATION_DIR=/path/to/bcm/app \
    -DREFERENCE_APP_SW_VERSION=...
cmake --build --preset posix-freertos --target app.referenceApp
```

## Maintaining the fork

- `main` mirrors upstream `main`.
- To pick up a newer upstream commit, rebase each topic branch onto it, then recreate `remotive/main` from that commit
  by merging the topic branches again. `remotive/main` is rebuilt this way, not rebased, so that every topic branch
  stays a clean series on top of upstream.
- `remotive/main` is rebuilt rather than fast-forwarded, so an old `remotive/main` commit ends up on no branch, and a
  build that pins it by SHA would eventually stop working. Builds therefore pin a tag instead. After rebuilding
  `remotive/main`, tag it `remotive/<YYYY-MM-DD>` (add `.1`, `.2` for more than one in a day), push the tag and point
  the BCM's `OPENBSW_REF` at it. Never move or delete a published tag: released versions of the BCM build from them.
- When a topic branch is merged upstream, drop it from the table above and from `remotive/main`.
- Contributions upstream need the Eclipse Contributor Agreement (ECA), signed with the author email of the commits.
  See [CONTRIBUTING.md](CONTRIBUTING.md).
- Commits on the topic branches follow upstream's [AGENTS.md](AGENTS.md) and the "Use of AI" section of
  CONTRIBUTING.md: no `Co-authored-by` trailers or "Generated with" footers.

## License

Like the rest of OpenBSW, the changes in this fork are under the Apache License 2.0, see [LICENSE](LICENSE).
