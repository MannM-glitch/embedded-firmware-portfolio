# Firmware Hiring Signals

Reviewed September 3, 2026. This is a portfolio design brief distilled from current firmware internship and entry-level roles, not a claim that any single project replaces professional hardware experience.

## Repeated Technical Signals

| Hiring signal | Project evidence |
| --- | --- |
| Embedded C and layered design | `controller.c` contains deterministic application logic; `hardware_sim.c` isolates the platform boundary. |
| ARM/peripheral literacy | The project models I2C-like sensor reads, GPIO register writes, UART-style telemetry, and watchdog servicing. |
| Real-time/RTOS thinking | The scenario runner invokes one bounded, periodic control cycle. State transitions and output behavior are deterministic. |
| Safety and debugging | A failed sensor transaction forces the fault state and a known-safe output register. The telemetry makes the failure visible. |
| Verification | Host-side tests assert hysteresis behavior, fault recovery, and watchdog servicing. GitHub Actions runs them on every push. |
| Communication | The architecture, behavior thresholds, build commands, and validation evidence are documented beside the source. |

## Current Roles Used As Design Inputs

1. [NXP: Firmware Engineer Intern, Summer 2026](https://nxp.wd3.myworkdayjobs.com/en-US/careers/job/Firmware-Engineer-Intern---Summer-2026_R-10062597) calls out C11/C99, ARM Cortex-M fundamentals, BSP/driver work for UART, I2C, SPI, GPIO, DMA and timers, plus Git, debugging, documentation, RTOS concepts, and test automation.
2. [Apple: Embedded Firmware Engineer Intern, Wireless Power Division](https://jobs.apple.com/en-il/details/200651594-0095/embedded-firmware-engineer-intern-wireless-power-division) asks for C/C++, scripting, microprocessor and RTOS knowledge, Git or unit testing, debugging, and technical communication.
3. [Fluxergy: Firmware Engineer Intern](https://jobs.lever.co/fluxergy-2/c592763e-56ba-4d20-b751-3a4574470eec) emphasizes C/C++, RTOS exposure, test harnesses, verification, error handling, logging, documentation, hardware-software troubleshooting, and Git.
4. [Western Digital: Intern, Firmware Engineering](https://jobs.smartrecruiters.com/WesternDigital/744000141840819-intern-firmware-engineering?trid=7d1dcdfa-96a8-4e55-bb9c-0db211f5a9b3) asks candidates to design, test, integrate, and maintain quality firmware with unit testing and CI; it also lists microcontroller, embedded-system, RTOS, C/C++, and Python knowledge.

## Design Decision

The project demonstrates a defensible embedded workflow: define an explicit safe state, isolate hardware access, use integer units rather than floating point for measurements, model realistic fault conditions, test behavior on the host, automate those tests, and expose observable telemetry. It is intentionally ready for a future hardware port rather than pretending a host simulation is a physical-board bring-up.
