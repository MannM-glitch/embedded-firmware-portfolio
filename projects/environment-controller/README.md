# Environmental Controller Firmware

A testable C firmware simulation of a safety-aware temperature and humidity controller. The project is deliberately structured so that its control logic can be retained while the simulated platform layer is replaced with a board-specific I2C, GPIO, UART, and watchdog implementation.

## Runtime Behavior

Every control cycle reads a simulated temperature/humidity sensor and writes an 8-bit GPIO-style output register:

| Output | GPIO bit | Behavior |
| --- | ---: | --- |
| Heater | 0 | Turns on at or below `18.0 C`; remains on until `20.0 C` |
| Fan | 1 | Turns on at or above `26.0 C`; remains on until `24.0 C` |
| Dehumidifier | 2 | Turns on at or above `65.0 %`; remains on until `60.0 %` |
| Alarm | 7 | Sole active output after a sensor-read failure |

The gap between an output's on and off threshold is hysteresis. It prevents actuator chatter when measurements hover near a limit.

## Architecture

```text
src/main.c              Scenario runner and UART-style telemetry
src/controller.c        Deterministic application state machine and safety policy
src/hardware_sim.c      Replaceable platform layer for I2C, GPIO, time, watchdog
tests/test_controller.c Host-side behavioral tests
```

## Build And Test

Windows PowerShell:

```powershell
.\build.ps1
.\build\environment-controller.exe
.\build\controller-tests.exe
```

GNU Make:

```text
make test
make run
```

## Evidence

- The executable runs nominal, cold, hot, high-humidity, sensor-fault, and recovery scenarios.
- Unit tests assert each hysteresis boundary plus the safe fault state and watchdog servicing.
- The browser dashboard at the repository root exposes the same thresholds, state transitions, GPIO bits, and fault behavior for a quick live review.
- [Job-posting research](docs/job-posting-research.md) maps this design to current firmware hiring signals.
