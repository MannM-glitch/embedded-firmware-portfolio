# Automotive Firmware Role Research

Reviewed September 3, 2026. The goal is to translate visible hiring requirements into a credible project scope without presenting simulation as vehicle production software.

## Tesla Signals

[Tesla's Embedded Firmware Engineer, Battery Management Systems role](https://www.tesla.com/careers/search/job/embedded-firmware-engineer-battery-management-systems-254793) calls out low-level firmware drivers, real-time software controllers and algorithms for BMS quantities such as SOC/SOH/SOE and power estimation, diagnostics, validation/test infrastructure, C/C++, scripting, real-time systems, microprocessor tooling, debugging, and direct hardware work.

[Tesla's current careers listing](https://www.tesla.com/careers/search/?query=internship&site=US) includes internship tracks in vehicle-firmware validation, firmware platforms, high-voltage systems, BMS integration, drive-system validation, and simulation. Tesla also lists active BMS and thermal-systems embedded roles in vehicle software.

## Rivian Signals

[Rivian's Embedded Software Engineer II, Charging & Energy role](https://careers.rivian.com/careers-home/jobs/32343) asks for modern C/C++, embedded Linux, Make/CMake, unit and SIL tests, CI/CD, scripting, CAN/Modbus, and charging-protocol exposure.

[Rivian's Lead Staff Embedded Autonomy role](https://careers.rivian.com/careers-home/jobs/30253?lang=en-us) lists applications, drivers, hardware bring-up, simulation, testing, safety/reliability, embedded C/C++, hardware/software interfaces, CAN/UDS/DoIP, and RTOS familiarity.

[Rivian's Staff Embedded Autonomy role](https://careers.rivian.com/careers-home/jobs/20371?lang=en-us) emphasizes resource-constrained real-time systems, kernels, multi-threaded debugging, CI/CD, tooling, and JTAG/logic-analyzer familiarity.

## Project Mapping

| Hiring signal | Vehicle Thermal ECU evidence |
| --- | --- |
| Real-time controller | Explicit 100 ms task with bounded state transitions |
| Driver/application boundary | Raw CAN pack/decode module is separate from the thermal policy |
| Vehicle communication | Timestamped 29-bit CAN frame plus a command response frame |
| Safety and diagnostics | Timeout supervision, thermal derate, safe shutdown, output fail-safe, DTC flags |
| Validation infrastructure | Deterministic C tests, portable Make build, GitHub Actions matrix |
| Simulation and observability | Native scenario executable plus a deployed dashboard with raw-frame and state telemetry |

## Honest Next Hardware Step

Port the CAN abstraction to a development board using SocketCAN, an MCP2515, or a native CAN-FD controller. Capture physical traffic with a CAN analyzer, preserve the test cases as hardware-in-the-loop scenarios, and document the measured timing. That would turn this software simulation into board-level evidence.
