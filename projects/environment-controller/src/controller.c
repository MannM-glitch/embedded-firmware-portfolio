#include "controller.h"

#include <stddef.h>

static uint8_t update_dehumidifier(uint8_t outputs, uint16_t humidity_tenths_percent)
{
    if (humidity_tenths_percent >= CONTROLLER_DEHUMIDIFIER_ON_TENTHS_PERCENT) {
        return outputs | HW_OUTPUT_DEHUMIDIFIER;
    }

    if (humidity_tenths_percent <= CONTROLLER_DEHUMIDIFIER_OFF_TENTHS_PERCENT) {
        return outputs & (uint8_t)~HW_OUTPUT_DEHUMIDIFIER;
    }

    return outputs;
}

void controller_init(Controller *controller)
{
    if (controller == NULL) {
        return;
    }

    controller->state = CONTROLLER_NORMAL;
    controller->last_outputs = 0u;
    controller->sensor_ok = false;
    controller->temperature_tenths_c = 0;
    controller->humidity_tenths_percent = 0u;
    controller->control_cycles = 0u;
}

void controller_run_cycle(Controller *controller, HardwareSim *hardware)
{
    int16_t temperature_tenths_c = 0;
    uint16_t humidity_tenths_percent = 0u;
    uint8_t outputs;
    bool temperature_ok;
    bool humidity_ok;

    if ((controller == NULL) || (hardware == NULL)) {
        return;
    }

    temperature_ok = hw_sim_read_temperature(hardware, &temperature_tenths_c);
    humidity_ok = hw_sim_read_humidity(hardware, &humidity_tenths_percent);
    controller->control_cycles += 1u;

    if (!temperature_ok || !humidity_ok) {
        controller->state = CONTROLLER_SENSOR_FAULT;
        controller->sensor_ok = false;
        controller->last_outputs = HW_OUTPUT_ALARM;
        hw_sim_write_outputs(hardware, controller->last_outputs);
        hw_sim_kick_watchdog(hardware);
        return;
    }

    controller->sensor_ok = true;
    controller->temperature_tenths_c = temperature_tenths_c;
    controller->humidity_tenths_percent = humidity_tenths_percent;

    if (controller->state == CONTROLLER_SENSOR_FAULT) {
        controller->state = CONTROLLER_NORMAL;
    }

    outputs = controller->last_outputs & HW_OUTPUT_DEHUMIDIFIER;

    switch (controller->state) {
    case CONTROLLER_NORMAL:
        if (temperature_tenths_c <= CONTROLLER_HEATER_ON_TENTHS_C) {
            controller->state = CONTROLLER_HEATING;
        } else if (temperature_tenths_c >= CONTROLLER_FAN_ON_TENTHS_C) {
            controller->state = CONTROLLER_COOLING;
        }
        break;

    case CONTROLLER_HEATING:
        if (temperature_tenths_c >= CONTROLLER_HEATER_OFF_TENTHS_C) {
            controller->state = CONTROLLER_NORMAL;
        }
        break;

    case CONTROLLER_COOLING:
        if (temperature_tenths_c <= CONTROLLER_FAN_OFF_TENTHS_C) {
            controller->state = CONTROLLER_NORMAL;
        }
        break;

    case CONTROLLER_SENSOR_FAULT:
        controller->state = CONTROLLER_NORMAL;
        break;
    }

    if (controller->state == CONTROLLER_HEATING) {
        outputs |= HW_OUTPUT_HEATER;
    } else if (controller->state == CONTROLLER_COOLING) {
        outputs |= HW_OUTPUT_FAN;
    }

    outputs = update_dehumidifier(outputs, humidity_tenths_percent);
    controller->last_outputs = outputs;
    hw_sim_write_outputs(hardware, outputs);
    hw_sim_kick_watchdog(hardware);
}

const char *controller_state_name(ControllerState state)
{
    switch (state) {
    case CONTROLLER_NORMAL:
        return "NORMAL";
    case CONTROLLER_HEATING:
        return "HEATING";
    case CONTROLLER_COOLING:
        return "COOLING";
    case CONTROLLER_SENSOR_FAULT:
        return "SENSOR_FAULT";
    }

    return "UNKNOWN";
}
