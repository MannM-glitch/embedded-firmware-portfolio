#include "hardware_sim.h"

#include <stddef.h>

void hw_sim_init(HardwareSim *hardware)
{
    if (hardware == NULL) {
        return;
    }

    hardware->gpio_output_register = 0u;
    hardware->temperature_tenths_c = 220;
    hardware->humidity_tenths_percent = 450u;
    hardware->sensor_online = true;
    hardware->time_ms = 0u;
    hardware->watchdog_kicks = 0u;
}

void hw_sim_advance_time(HardwareSim *hardware, uint32_t elapsed_ms)
{
    if (hardware != NULL) {
        hardware->time_ms += elapsed_ms;
    }
}

void hw_sim_set_sensor_sample(HardwareSim *hardware,
                              bool sensor_online,
                              int16_t temperature_tenths_c,
                              uint16_t humidity_tenths_percent)
{
    if (hardware == NULL) {
        return;
    }

    hardware->sensor_online = sensor_online;
    hardware->temperature_tenths_c = temperature_tenths_c;
    hardware->humidity_tenths_percent = humidity_tenths_percent;
}

bool hw_sim_read_temperature(const HardwareSim *hardware, int16_t *temperature_tenths_c)
{
    if ((hardware == NULL) || (temperature_tenths_c == NULL) || !hardware->sensor_online) {
        return false;
    }

    *temperature_tenths_c = hardware->temperature_tenths_c;
    return true;
}

bool hw_sim_read_humidity(const HardwareSim *hardware, uint16_t *humidity_tenths_percent)
{
    if ((hardware == NULL) || (humidity_tenths_percent == NULL) || !hardware->sensor_online) {
        return false;
    }

    *humidity_tenths_percent = hardware->humidity_tenths_percent;
    return true;
}

void hw_sim_write_outputs(HardwareSim *hardware, uint8_t output_mask)
{
    if (hardware != NULL) {
        hardware->gpio_output_register = output_mask;
    }
}

void hw_sim_kick_watchdog(HardwareSim *hardware)
{
    if (hardware != NULL) {
        hardware->watchdog_kicks += 1u;
    }
}
