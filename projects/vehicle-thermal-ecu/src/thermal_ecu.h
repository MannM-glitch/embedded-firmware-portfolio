#ifndef THERMAL_ECU_H
#define THERMAL_ECU_H

#include <stdbool.h>
#include <stdint.h>

#include "can_bus_sim.h"

enum {
    THERMAL_TASK_PERIOD_MS = 100u,
    THERMAL_BMS_TIMEOUT_MS = 300u,
    THERMAL_COOL_ON_TENTHS_C = 500,
    THERMAL_COOL_OFF_TENTHS_C = 450,
    THERMAL_DERATE_ON_TENTHS_C = 600,
    THERMAL_DERATE_OFF_TENTHS_C = 570,
    THERMAL_SHUTDOWN_ON_TENTHS_C = 650,
    THERMAL_PRECONDITION_ON_TENTHS_C = 50,
    THERMAL_PRECONDITION_OFF_TENTHS_C = 100,
    THERMAL_DERATED_POWER_LIMIT_PERCENT = 50u
};

enum {
    THERMAL_OUTPUT_COOLANT_PUMP = 1u << 0,
    THERMAL_OUTPUT_RADIATOR_FAN = 1u << 1,
    THERMAL_OUTPUT_PACK_HEATER = 1u << 2,
    THERMAL_OUTPUT_CHARGE_ENABLE = 1u << 3,
    THERMAL_OUTPUT_FAULT_LAMP = 1u << 7
};

enum {
    THERMAL_DTC_NONE = 0u,
    THERMAL_DTC_BMS_CAN_TIMEOUT = 1u << 0,
    THERMAL_DTC_POWER_DERATE = 1u << 1,
    THERMAL_DTC_CRITICAL_PACK_TEMPERATURE = 1u << 2
};

typedef enum {
    THERMAL_ECU_NORMAL = 0u,
    THERMAL_ECU_PRECONDITIONING = 1u,
    THERMAL_ECU_COOLING = 2u,
    THERMAL_ECU_POWER_DERATE = 3u,
    THERMAL_ECU_SAFE_SHUTDOWN = 4u,
    THERMAL_ECU_BMS_TIMEOUT = 5u
} ThermalEcuState;

typedef struct {
    ThermalEcuState state;
    uint8_t output_mask;
    uint8_t torque_limit_percent;
    uint8_t charge_limit_percent;
    uint8_t dtc_flags;
    bool bms_status_fresh;
    int16_t pack_temperature_tenths_c;
    uint16_t pack_voltage_deci_v;
    VehicleMode vehicle_mode;
    uint32_t task_runs;
    uint32_t watchdog_kicks;
} ThermalEcu;

void thermal_ecu_init(ThermalEcu *ecu);
void thermal_ecu_run_100ms(ThermalEcu *ecu, CanBusSim *bus, uint32_t now_ms);
const char *thermal_ecu_state_name(ThermalEcuState state);

#endif
