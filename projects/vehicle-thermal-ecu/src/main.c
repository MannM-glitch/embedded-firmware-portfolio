#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "can_bus_sim.h"
#include "thermal_ecu.h"

typedef struct {
    const char *label;
    bool inject_bms_frame;
    uint32_t elapsed_ms;
    int16_t pack_temperature_tenths_c;
    uint16_t pack_voltage_deci_v;
    VehicleMode vehicle_mode;
} ScenarioStep;

static void print_temperature(int16_t temperature_tenths_c)
{
    int temperature = (int)temperature_tenths_c;
    int fraction = temperature % 10;

    if (fraction < 0) {
        fraction = -fraction;
    }

    printf("%d.%d C", temperature / 10, fraction);
}

static void print_frame_data(const CanFrame *frame)
{
    for (uint8_t index = 0u; index < frame->dlc; ++index) {
        printf("%02X%s", (unsigned int)frame->data[index],
               (index + 1u == frame->dlc) ? "" : " ");
    }
}

static void print_cycle(const ThermalEcu *ecu, const CanBusSim *bus, const ScenarioStep *step,
                        uint32_t now_ms)
{
    printf("[%04lu ms] %-20s State: %-17s ", (unsigned long)now_ms, step->label,
           thermal_ecu_state_name(ecu->state));

    if (ecu->bms_status_fresh) {
        printf("Pack: ");
        print_temperature(ecu->pack_temperature_tenths_c);
        printf("  %u.%u V  ", (unsigned int)(ecu->pack_voltage_deci_v / 10u),
               (unsigned int)(ecu->pack_voltage_deci_v % 10u));
    } else {
        printf("Pack: stale            ");
    }

    printf("Torque: %3u%% Charge: %3u%% DTC: 0x%02X\n",
           (unsigned int)ecu->torque_limit_percent, (unsigned int)ecu->charge_limit_percent,
           (unsigned int)ecu->dtc_flags);
    printf("            TX 0x%08lX [", (unsigned long)bus->last_tx.id);
    print_frame_data(&bus->last_tx);
    printf("]\n");
}

int main(void)
{
    static const ScenarioStep scenario[] = {
        {"nominal drive", true, 0u, 230, 4000u, VEHICLE_MODE_DRIVE},
        {"thermal cooling", true, 100u, 520, 3990u, VEHICLE_MODE_DRIVE},
        {"power derate", true, 100u, 610, 3980u, VEHICLE_MODE_DRIVE},
        {"critical temperature", true, 100u, 660, 3970u, VEHICLE_MODE_DRIVE},
        {"recovered pack", true, 100u, 430, 3995u, VEHICLE_MODE_DRIVE},
        {"cold charge request", true, 100u, 30, 4010u, VEHICLE_MODE_CHARGE},
        {"charge ready", true, 100u, 110, 4020u, VEHICLE_MODE_CHARGE},
        {"BMS timeout", false, 300u, 0, 0u, VEHICLE_MODE_DRIVE},
        {"BMS recovered", true, 100u, 240, 4000u, VEHICLE_MODE_DRIVE}
    };
    CanBusSim bus;
    ThermalEcu ecu;
    uint32_t now_ms = 0u;

    can_bus_sim_init(&bus);
    thermal_ecu_init(&ecu);

    printf("Vehicle Battery Thermal ECU Simulation\n");
    printf("======================================\n");

    for (size_t step_index = 0u; step_index < (sizeof(scenario) / sizeof(scenario[0])); ++step_index) {
        const ScenarioStep *step = &scenario[step_index];

        now_ms += step->elapsed_ms;
        if (step->inject_bms_frame) {
            can_bus_sim_inject_bms_status(&bus, step->pack_temperature_tenths_c,
                                          step->pack_voltage_deci_v, step->vehicle_mode, now_ms);
        }
        thermal_ecu_run_100ms(&ecu, &bus, now_ms);
        print_cycle(&ecu, &bus, step, now_ms);
    }

    printf("\nTask runs: %lu  Watchdog kicks: %lu  Command frames: %lu\n",
           (unsigned long)ecu.task_runs, (unsigned long)ecu.watchdog_kicks,
           (unsigned long)bus.tx_count);
    return 0;
}
