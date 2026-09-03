#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware_sim.h"

enum {
    CONTROLLER_HEATER_ON_TENTHS_C = 180,
    CONTROLLER_HEATER_OFF_TENTHS_C = 200,
    CONTROLLER_FAN_ON_TENTHS_C = 260,
    CONTROLLER_FAN_OFF_TENTHS_C = 240,
    CONTROLLER_DEHUMIDIFIER_ON_TENTHS_PERCENT = 650,
    CONTROLLER_DEHUMIDIFIER_OFF_TENTHS_PERCENT = 600
};

typedef enum {
    CONTROLLER_NORMAL,
    CONTROLLER_HEATING,
    CONTROLLER_COOLING,
    CONTROLLER_SENSOR_FAULT
} ControllerState;

typedef struct {
    ControllerState state;
    uint8_t last_outputs;
    bool sensor_ok;
    int16_t temperature_tenths_c;
    uint16_t humidity_tenths_percent;
    uint32_t control_cycles;
} Controller;

void controller_init(Controller *controller);
void controller_run_cycle(Controller *controller, HardwareSim *hardware);
const char *controller_state_name(ControllerState state);

#endif
