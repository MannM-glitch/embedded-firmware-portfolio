#include <assert.h>
#include <stdio.h>

#include "can_bus_sim.h"
#include "thermal_ecu.h"

static void run_with_bms(ThermalEcu *ecu,
                         CanBusSim *bus,
                         uint32_t now_ms,
                         int16_t pack_temperature_tenths_c,
                         VehicleMode vehicle_mode)
{
    can_bus_sim_inject_bms_status(bus, pack_temperature_tenths_c, 4000u, vehicle_mode, now_ms);
    thermal_ecu_run_100ms(ecu, bus, now_ms);
}

static void test_nominal_and_cooling(void)
{
    CanBusSim bus;
    ThermalEcu ecu;

    can_bus_sim_init(&bus);
    thermal_ecu_init(&ecu);

    run_with_bms(&ecu, &bus, 0u, 240, VEHICLE_MODE_DRIVE);
    assert(ecu.state == THERMAL_ECU_NORMAL);
    assert(ecu.output_mask == 0u);
    assert(ecu.torque_limit_percent == 100u);

    run_with_bms(&ecu, &bus, 100u, 520, VEHICLE_MODE_DRIVE);
    assert(ecu.state == THERMAL_ECU_COOLING);
    assert(ecu.output_mask == (THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_RADIATOR_FAN));
    assert(bus.last_tx.id == CAN_ID_THERMAL_COMMAND);
    assert(bus.last_tx.data[0] == ecu.output_mask);
}

static void test_derate_and_shutdown(void)
{
    CanBusSim bus;
    ThermalEcu ecu;

    can_bus_sim_init(&bus);
    thermal_ecu_init(&ecu);

    run_with_bms(&ecu, &bus, 0u, 610, VEHICLE_MODE_DRIVE);
    assert(ecu.state == THERMAL_ECU_POWER_DERATE);
    assert(ecu.torque_limit_percent == THERMAL_DERATED_POWER_LIMIT_PERCENT);
    assert((ecu.dtc_flags & THERMAL_DTC_POWER_DERATE) != 0u);

    run_with_bms(&ecu, &bus, 100u, 660, VEHICLE_MODE_DRIVE);
    assert(ecu.state == THERMAL_ECU_SAFE_SHUTDOWN);
    assert(ecu.torque_limit_percent == 0u);
    assert(ecu.charge_limit_percent == 0u);
    assert((ecu.output_mask & THERMAL_OUTPUT_FAULT_LAMP) != 0u);
    assert((ecu.dtc_flags & THERMAL_DTC_CRITICAL_PACK_TEMPERATURE) != 0u);
}

static void test_can_timeout(void)
{
    CanBusSim bus;
    ThermalEcu ecu;

    can_bus_sim_init(&bus);
    thermal_ecu_init(&ecu);

    run_with_bms(&ecu, &bus, 0u, 250, VEHICLE_MODE_DRIVE);
    thermal_ecu_run_100ms(&ecu, &bus, THERMAL_BMS_TIMEOUT_MS);

    assert(ecu.state == THERMAL_ECU_BMS_TIMEOUT);
    assert(!ecu.bms_status_fresh);
    assert(ecu.torque_limit_percent == 0u);
    assert((ecu.output_mask & THERMAL_OUTPUT_FAULT_LAMP) != 0u);
    assert((ecu.dtc_flags & THERMAL_DTC_BMS_CAN_TIMEOUT) != 0u);
}

static void test_charge_preconditioning(void)
{
    CanBusSim bus;
    ThermalEcu ecu;

    can_bus_sim_init(&bus);
    thermal_ecu_init(&ecu);

    run_with_bms(&ecu, &bus, 0u, 30, VEHICLE_MODE_CHARGE);
    assert(ecu.state == THERMAL_ECU_PRECONDITIONING);
    assert(ecu.charge_limit_percent == 0u);
    assert(ecu.output_mask == (THERMAL_OUTPUT_COOLANT_PUMP | THERMAL_OUTPUT_PACK_HEATER));

    run_with_bms(&ecu, &bus, 100u, 110, VEHICLE_MODE_CHARGE);
    assert(ecu.state == THERMAL_ECU_NORMAL);
    assert(ecu.output_mask == THERMAL_OUTPUT_CHARGE_ENABLE);
    assert(ecu.charge_limit_percent == 100u);
}

int main(void)
{
    test_nominal_and_cooling();
    test_derate_and_shutdown();
    test_can_timeout();
    test_charge_preconditioning();

    printf("All vehicle thermal ECU tests passed.\n");
    return 0;
}
