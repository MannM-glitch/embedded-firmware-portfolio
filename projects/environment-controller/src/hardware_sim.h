#ifndef HARDWARE_SIM_H
#define HARDWARE_SIM_H

#include <stdbool.h>
#include <stdint.h>

enum {
    HW_OUTPUT_HEATER = 1u << 0,
    HW_OUTPUT_FAN = 1u << 1,
    HW_OUTPUT_DEHUMIDIFIER = 1u << 2,
    HW_OUTPUT_ALARM = 1u << 7
};

typedef struct {
    uint8_t gpio_output_register;
    int16_t temperature_tenths_c;
    uint16_t humidity_tenths_percent;
    bool sensor_online;
    uint32_t time_ms;
    uint32_t watchdog_kicks;
} HardwareSim;

void hw_sim_init(HardwareSim *hardware);
void hw_sim_advance_time(HardwareSim *hardware, uint32_t elapsed_ms);
void hw_sim_set_sensor_sample(HardwareSim *hardware,
                              bool sensor_online,
                              int16_t temperature_tenths_c,
                              uint16_t humidity_tenths_percent);
bool hw_sim_read_temperature(const HardwareSim *hardware, int16_t *temperature_tenths_c);
bool hw_sim_read_humidity(const HardwareSim *hardware, uint16_t *humidity_tenths_percent);
void hw_sim_write_outputs(HardwareSim *hardware, uint8_t output_mask);
void hw_sim_kick_watchdog(HardwareSim *hardware);

#endif
