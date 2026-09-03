#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "controller.h"
#include "hardware_sim.h"

typedef struct {
    const char *label;
    bool sensor_online;
    int16_t temperature_tenths_c;
    uint16_t humidity_tenths_percent;
} ScenarioStep;

static void print_binary8(uint8_t value)
{
    for (int bit_index = 7; bit_index >= 0; --bit_index) {
        unsigned int bit = ((unsigned int)value >> bit_index) & 1u;
        printf("%u", bit);
    }
}

static void print_temperature(int16_t temperature_tenths_c)
{
    int temperature = (int)temperature_tenths_c;
    int fraction = temperature % 10;

    if (fraction < 0) {
        fraction = -fraction;
    }

    printf("%d.%d C", temperature / 10, fraction);
}

static void print_outputs(uint8_t outputs)
{
    bool first = true;

    if (outputs == 0u) {
        printf("none");
        return;
    }

    if ((outputs & HW_OUTPUT_HEATER) != 0u) {
        printf("HEATER");
        first = false;
    }
    if ((outputs & HW_OUTPUT_FAN) != 0u) {
        printf("%sFAN", first ? "" : ", ");
        first = false;
    }
    if ((outputs & HW_OUTPUT_DEHUMIDIFIER) != 0u) {
        printf("%sDEHUM", first ? "" : ", ");
        first = false;
    }
    if ((outputs & HW_OUTPUT_ALARM) != 0u) {
        printf("%sALARM", first ? "" : ", ");
    }
}

static void print_cycle(const HardwareSim *hardware, const Controller *controller, const ScenarioStep *step)
{
    printf("[%06lu ms] %-24s ", (unsigned long)hardware->time_ms, step->label);
    printf("I2C: %-4s ", controller->sensor_ok ? "OK" : "FAIL");

    if (controller->sensor_ok) {
        printf("Temp: ");
        print_temperature(controller->temperature_tenths_c);
        printf("  Humidity: %u.%u %%  ",
               (unsigned int)(controller->humidity_tenths_percent / 10u),
               (unsigned int)(controller->humidity_tenths_percent % 10u));
    } else {
        printf("Temp: --.- C  Humidity: --.- %%  ");
    }

    printf("Mode: %-12s GPIO: 0b", controller_state_name(controller->state));
    print_binary8(hardware->gpio_output_register);
    printf("  Outputs: ");
    print_outputs(hardware->gpio_output_register);
    printf("\n");
}

int main(void)
{
    static const ScenarioStep scenario[] = {
        {"startup", true, 220, 450u},
        {"cold room", true, 165, 450u},
        {"warming up", true, 195, 450u},
        {"comfortable", true, 205, 450u},
        {"high humidity", true, 225, 710u},
        {"humidity recovered", true, 225, 580u},
        {"over-temperature", true, 275, 580u},
        {"cooling complete", true, 235, 580u},
        {"sensor cable unplugged", false, 0, 0u},
        {"sensor recovered", true, 225, 500u}
    };
    HardwareSim hardware;
    Controller controller;

    hw_sim_init(&hardware);
    controller_init(&controller);

    printf("Environmental Controller Firmware Simulation\n");
    printf("=============================================\n");

    for (size_t step_index = 0u; step_index < (sizeof(scenario) / sizeof(scenario[0])); ++step_index) {
        const ScenarioStep *step = &scenario[step_index];

        hw_sim_set_sensor_sample(&hardware,
                                 step->sensor_online,
                                 step->temperature_tenths_c,
                                 step->humidity_tenths_percent);
        controller_run_cycle(&controller, &hardware);
        print_cycle(&hardware, &controller, step);
        hw_sim_advance_time(&hardware, 1000u);
    }

    printf("\nControl cycles: %lu  Watchdog kicks: %lu\n",
           (unsigned long)controller.control_cycles,
           (unsigned long)hardware.watchdog_kicks);
    return 0;
}
