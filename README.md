# Embedded Firmware Portfolio

Portfolio projects for embedded and firmware engineering roles. Each project is designed around production signals found in current internship and entry-level postings: C/C++, peripheral boundaries, deterministic behavior, testing, fault handling, debugging evidence, and clear technical documentation.

## Featured Project: Environmental Controller Firmware

[View the firmware source](projects/environment-controller/src) | [Read the architecture](projects/environment-controller/README.md)

A safety-aware environmental controller written in C. The project models a periodic control task that reads a sensor over a simulated I2C boundary and drives an 8-bit GPIO output register for a heater, fan, dehumidifier, and fault alarm.

Highlights:

- Hysteresis-based thermal and humidity control to prevent rapid actuator switching
- Fail-safe behavior for sensor loss: deactivate process outputs and assert the alarm bit
- Hardware abstraction boundary for I2C-like reads, GPIO writes, watchdog servicing, and time
- Host-side unit tests for thermal control, humidity control, sensor fault recovery, and watchdog servicing
- GitHub Actions CI verification on every push
- An interactive deployed dashboard that reproduces the controller's scenario behavior and exposes live state, register bits, and event telemetry

## Repository Layout

```text
projects/environment-controller/
  src/          Firmware application and platform-simulation layers
  tests/        Host-side behavioral tests
  docs/         Architecture and job-signal research
showcase/       Deployed interactive controller dashboard
```

## Verification

```powershell
cd projects\environment-controller
.\build.ps1
.\build\controller-tests.exe
```

The project also supports `make test` on systems with GNU Make and GCC.
