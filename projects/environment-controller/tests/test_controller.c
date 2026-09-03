#include <assert.h>
#include <stdio.h>

#include "controller.h"
#include "hardware_sim.h"

static void run_sample(Controller *controller,
                       HardwareSim *hardware,
                       int16_t temperature_tenths_c,
                       uint16_t humidity_tenths_percent)
{
    hw_sim_set_sensor_sample(hardware, true, temperature_tenths_c, humidity_tenths_percent);
    controller_run_cycle(controller, hardware);
}

static void test_thermal_hysteresis(void)
{
    HardwareSim hardware;
    Controller controller;

    hw_sim_init(&hardware);
    controller_init(&controller);

    run_sample(&controller, &hardware, 175, 450u);
    assert(controller.state == CONTROLLER_HEATING);
    assert(hardware.gpio_output_register == HW_OUTPUT_HEATER);

    run_sample(&controller, &hardware, 190, 450u);
    assert(controller.state == CONTROLLER_HEATING);
    assert(hardware.gpio_output_register == HW_OUTPUT_HEATER);

    run_sample(&controller, &hardware, 205, 450u);
    assert(controller.state == CONTROLLER_NORMAL);
    assert(hardware.gpio_output_register == 0u);

    run_sample(&controller, &hardware, 270, 450u);
    assert(controller.state == CONTROLLER_COOLING);
    assert(hardware.gpio_output_register == HW_OUTPUT_FAN);

    run_sample(&controller, &hardware, 235, 450u);
    assert(controller.state == CONTROLLER_NORMAL);
    assert(hardware.gpio_output_register == 0u);
}

static void test_humidity_control(void)
{
    HardwareSim hardware;
    Controller controller;

    hw_sim_init(&hardware);
    controller_init(&controller);

    run_sample(&controller, &hardware, 220, 700u);
    assert((hardware.gpio_output_register & HW_OUTPUT_DEHUMIDIFIER) != 0u);

    run_sample(&controller, &hardware, 220, 625u);
    assert((hardware.gpio_output_register & HW_OUTPUT_DEHUMIDIFIER) != 0u);

    run_sample(&controller, &hardware, 220, 590u);
    assert((hardware.gpio_output_register & HW_OUTPUT_DEHUMIDIFIER) == 0u);
}

static void test_sensor_fault_and_recovery(void)
{
    HardwareSim hardware;
    Controller controller;

    hw_sim_init(&hardware);
    controller_init(&controller);

    hw_sim_set_sensor_sample(&hardware, false, 0, 0u);
    controller_run_cycle(&controller, &hardware);
    assert(controller.state == CONTROLLER_SENSOR_FAULT);
    assert(!controller.sensor_ok);
    assert(hardware.gpio_output_register == HW_OUTPUT_ALARM);

    run_sample(&controller, &hardware, 220, 450u);
    assert(controller.state == CONTROLLER_NORMAL);
    assert(controller.sensor_ok);
    assert(hardware.gpio_output_register == 0u);
    assert(hardware.watchdog_kicks == 2u);
}

int main(void)
{
    test_thermal_hysteresis();
    test_humidity_control();
    test_sensor_fault_and_recovery();

    printf("All controller tests passed.\n");
    return 0;
}
