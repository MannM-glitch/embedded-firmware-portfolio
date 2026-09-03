# Embedded Firmware Portfolio

Public portfolio projects for embedded and vehicle-software roles. The projects emphasize deterministic C/C++-style architecture, hardware boundaries, vehicle communication, safety supervision, observability, validation, and repeatable builds.

## Featured: Vehicle Battery Thermal ECU

[Open the interactive ECU console](https://manna-embedded-firmware.blithe-flint-7051.chatgpt.site) | [View the C source](projects/vehicle-thermal-ecu/src) | [Read the interface and safety policy](projects/vehicle-thermal-ecu/README.md)

A C11 battery thermal-management ECU simulation that consumes a timestamped 29-bit BMS CAN frame every 100 ms and publishes a thermal-command frame. It implements cooling, thermal power derating, safe shutdown, cold-charge preconditioning, timeout supervision, DTC flags, a watchdog counter, and host-side tests.

Portfolio evidence:

- Independent eight-byte CAN frame packing, decoding, freshness validation, and command publication
- Explicit thermal state machine with hysteresis and safe output behavior
- Power authority and charging authority limits as first-class command signals
- BMS message timeout handling that commands a conservative fail-safe state
- Native C tests for command behavior and safety transitions
- GitHub Actions CI matrix for every portfolio project
- A deployed CAN trace console that makes raw frame data, state, diagnostics, and output bits reviewable without building locally

This design is informed by current [Tesla BMS firmware](https://www.tesla.com/careers/search/job/embedded-firmware-engineer-battery-management-systems-254793) and [Rivian embedded vehicle-platform](https://careers.rivian.com/careers-home/jobs/30253?lang=en-us) requirements, but uses independent interfaces and simulation thresholds.

## Additional Project: Environmental Controller Firmware

[Open the simulation](https://manna-embedded-firmware.blithe-flint-7051.chatgpt.site/environmental-controller.html) | [View the C source](projects/environment-controller/src)

A safety-aware temperature and humidity controller in C with an I2C-like sensor boundary, GPIO register outputs, watchdog servicing, hysteresis, and host-side unit tests.

## Repository Layout

```text
projects/vehicle-thermal-ecu/
  src/          CAN transport boundary and thermal ECU state machine
  tests/        Safety and command-publication tests
  docs/         Tesla/Rivian role research and project mapping
projects/environment-controller/
  src/          Earlier sensor-and-actuator firmware project
dist/           Deployed interactive firmware consoles
```

## Verification

Each project supports a strict PowerShell build and a portable GNU Make test target:

```powershell
cd projects\vehicle-thermal-ecu
.\build.ps1
.\build\thermal-ecu-tests.exe
```

```text
make test
```
