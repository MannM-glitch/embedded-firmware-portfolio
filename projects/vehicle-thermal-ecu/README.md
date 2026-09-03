# Vehicle Battery Thermal ECU

A C11 automotive firmware simulation for a battery thermal-management ECU. The project treats BMS data as timestamped CAN traffic, executes a deterministic 100 ms control task, publishes a thermal command frame, and transitions to safe behavior when the BMS message becomes stale.

## Behavior

| State | Entry condition | Command |
| --- | --- | --- |
| `NORMAL` | Fresh BMS frame and pack within the operating envelope | 100% torque/charge authority; enable charging during charge mode |
| `PRECONDITIONING` | Charge request with pack at or below `5.0 C` | Coolant pump and pack heater; charge limit 0% until `10.0 C` |
| `COOLING` | Pack at or above `50.0 C` | Coolant pump and radiator fan; hysteresis holds to `45.0 C` |
| `POWER_DERATE` | Pack at or above `60.0 C` | Pump/fan; torque and charge limited to 50%; diagnostic flag set |
| `SAFE_SHUTDOWN` | Pack at or above `65.0 C` | Pump/fan/fault lamp; torque and charge limited to 0% |
| `BMS_TIMEOUT` | No valid BMS frame for 300 ms | Pump/fan/fault lamp; torque and charge limited to 0% |

These are portfolio simulation thresholds, not vehicle calibration values.

## CAN Interface

The project uses independent, simulated 29-bit identifiers rather than OEM-proprietary definitions.

| Direction | Identifier | Payload |
| --- | ---: | --- |
| BMS to ECU | `0x18FF50E5` | Pack temperature in 0.1 C, pack voltage in 0.1 V, drive/charge mode |
| ECU to vehicle | `0x18FF51E5` | Output mask, torque limit, charge limit, state, DTC flags, rolling counter |

`src/can_bus_sim.c` packs and decodes eight-byte frames. `src/thermal_ecu.c` owns the application state machine and never depends on the simulator UI.

## Build And Test

Windows PowerShell:

```powershell
.\build.ps1
.\build\thermal-ecu-tests.exe
.\build\vehicle-thermal-ecu.exe
```

GNU Make:

```text
make test
make run
```

## Verification

The host-side tests cover nominal operation, cooling command publication, power derating, critical shutdown, stale-CAN supervision, charge preconditioning, and charge enable after thermal recovery.

The [interactive dashboard](https://manna-embedded-firmware.blithe-flint-7051.chatgpt.site) exposes the same state transitions, raw frames, freshness timer, command payload, outputs, and DTCs.

## Role Alignment

Tesla's [BMS firmware role](https://www.tesla.com/careers/search/job/embedded-firmware-engineer-battery-management-systems-254793) names low-level drivers, real-time controllers, safety/reliability, diagnostics, and validation/test infrastructure. Rivian's [embedded autonomy role](https://careers.rivian.com/careers-home/jobs/30253?lang=en-us) names C/C++, hardware bring-up, simulation, CAN/UDS/DoIP, testing, safety, and documentation. This project demonstrates a small, independently designed slice of those engineering practices.
