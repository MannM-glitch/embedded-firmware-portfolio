#include "thermal_ecu.h"

#include <stddef.h>

static void publish_command(ThermalEcu *ecu, CanBusSim *bus, uint32_t now_ms)
{
    can_bus_sim_publish_thermal_command(bus,
                                        ecu->output_mask,
                                        ecu->torque_limit_percent,
                                        ecu->charge_limit_percent,
                                        (uint8_t)ecu->state,
                                        ecu->dtc_flags,
                                        now_ms);
}

static bool cooling_is_latched(const ThermalEcu *ecu)
{
    return (ecu->state == THERMAL_ECU_COOLING) || (ecu->state == THERMAL_ECU_POWER_DERATE);
}

void thermal_ecu_init(ThermalEcu *ecu)
{
    if (ecu == NULL) {
        return;
    }

    ecu->state = THERMAL_ECU_NORMAL;
    ecu->output_mask = 0u;
    ecu->torque_limit_percent = 100u;
    ecu->charge_limit_percent = 100u;
    ecu->dtc_flags = THERMAL_DTC_NONE;
    ecu->bms_status_fresh = false;
    ecu->pack_temperature_tenths_c = 0;
    ecu->pack_voltage_deci_v = 0u;
    ecu->vehicle_mode = VEHICLE_MODE_DRIVE;
    ecu->task_runs = 0u;
    ecu->watchdog_kicks = 0u;
}

void thermal_ecu_run_100ms(ThermalEcu *ecu, CanBusSim *bus, uint32_t now_ms)
{
    BmsThermalStatus status;
    bool needs_cooling;
    bool needs_derate;
    bool needs_preconditioning;

    if ((ecu == NULL) || (bus == NULL)) {
        return;
    }

    ecu->task_runs += 1u;
    ecu->watchdog_kicks += 1u;

    if (!can_bus_sim_read_fresh_bms_status(bus, now_ms, THERMAL_BMS_TIMEOUT_MS, &status)) {
        ecu->state = THERMAL_ECU_BMS_TIMEOUT;
        ecu->output_mask = THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_RADIATOR_FAN |
                           THERMAL_OUTPUT_FAULT_LAMP;
        ecu->torque_limit_percent = 0u;
        ecu->charge_limit_percent = 0u;
        ecu->dtc_flags = THERMAL_DTC_BMS_CAN_TIMEOUT;
        ecu->bms_status_fresh = false;
        publish_command(ecu, bus, now_ms);
        return;
    }

    ecu->bms_status_fresh = true;
    ecu->pack_temperature_tenths_c = status.pack_temperature_tenths_c;
    ecu->pack_voltage_deci_v = status.pack_voltage_deci_v;
    ecu->vehicle_mode = status.vehicle_mode;
    ecu->output_mask = 0u;
    ecu->torque_limit_percent = 100u;
    ecu->charge_limit_percent = 100u;
    ecu->dtc_flags = THERMAL_DTC_NONE;

    needs_preconditioning = (status.vehicle_mode == VEHICLE_MODE_CHARGE) &&
                            ((status.pack_temperature_tenths_c <= THERMAL_PRECONDITION_ON_TENTHS_C) ||
                             ((ecu->state == THERMAL_ECU_PRECONDITIONING) &&
                              (status.pack_temperature_tenths_c < THERMAL_PRECONDITION_OFF_TENTHS_C)));
    needs_derate = (status.pack_temperature_tenths_c >= THERMAL_DERATE_ON_TENTHS_C) ||
                    ((ecu->state == THERMAL_ECU_POWER_DERATE) &&
                     (status.pack_temperature_tenths_c >= THERMAL_DERATE_OFF_TENTHS_C));
    needs_cooling = (status.pack_temperature_tenths_c >= THERMAL_COOL_ON_TENTHS_C) ||
                    (cooling_is_latched(ecu) &&
                     (status.pack_temperature_tenths_c >= THERMAL_COOL_OFF_TENTHS_C));

    if (status.pack_temperature_tenths_c >= THERMAL_SHUTDOWN_ON_TENTHS_C) {
        ecu->state = THERMAL_ECU_SAFE_SHUTDOWN;
        ecu->output_mask = THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_RADIATOR_FAN |
                           THERMAL_OUTPUT_FAULT_LAMP;
        ecu->torque_limit_percent = 0u;
        ecu->charge_limit_percent = 0u;
        ecu->dtc_flags = THERMAL_DTC_CRITICAL_PACK_TEMPERATURE;
    } else if (needs_preconditioning) {
        ecu->state = THERMAL_ECU_PRECONDITIONING;
        ecu->output_mask = THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_PACK_HEATER;
        ecu->charge_limit_percent = 0u;
    } else if (needs_derate) {
        ecu->state = THERMAL_ECU_POWER_DERATE;
        ecu->output_mask = THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_RADIATOR_FAN;
        ecu->torque_limit_percent = THERMAL_DERATED_POWER_LIMIT_PERCENT;
        ecu->charge_limit_percent = THERMAL_DERATED_POWER_LIMIT_PERCENT;
        ecu->dtc_flags = THERMAL_DTC_POWER_DERATE;
    } else if (needs_cooling) {
        ecu->state = THERMAL_ECU_COOLING;
        ecu->output_mask = THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_RADIATOR_FAN;
    } else {
        ecu->state = THERMAL_ECU_NORMAL;
        if (status.vehicle_mode == VEHICLE_MODE_CHARGE) {
            ecu->output_mask = THERMAL_OUTPUT_CHARGE_ENABLE;
        }
    }

    publish_command(ecu, bus, now_ms);
}

const char *thermal_ecu_state_name(ThermalEcuState state)
{
    switch (state) {
    case THERMAL_ECU_NORMAL:
        return "NORMAL";
    case THERMAL_ECU_PRECONDITIONING:
        return "PRECONDITIONING";
    case THERMAL_ECU_COOLING:
        return "COOLING";
    case THERMAL_ECU_POWER_DERATE:
        return "POWER_DERATE";
    case THERMAL_ECU_SAFE_SHUTDOWN:
        return "SAFE_SHUTDOWN";
    case THERMAL_ECU_BMS_TIMEOUT:
        return "BMS_TIMEOUT";
    }

    return "UNKNOWN";
}
