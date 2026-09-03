#include "can_bus_sim.h"

#include <stddef.h>

static uint16_t read_u16_le(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8u);
}

static void write_u16_le(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)(value & 0xFFu);
    bytes[1] = (uint8_t)(value >> 8u);
}

void can_bus_sim_init(CanBusSim *bus)
{
    if (bus == NULL) {
        return;
    }

    bus->has_bms_status = false;
    bus->last_rx = (CanFrame){0};
    bus->last_tx = (CanFrame){0};
    bus->tx_count = 0u;
}

void can_bus_sim_inject_bms_status(CanBusSim *bus,
                                   int16_t pack_temperature_tenths_c,
                                   uint16_t pack_voltage_deci_v,
                                   VehicleMode vehicle_mode,
                                   uint32_t timestamp_ms)
{
    if (bus == NULL) {
        return;
    }

    bus->last_rx.id = CAN_ID_BMS_THERMAL_STATUS;
    bus->last_rx.dlc = CAN_FRAME_DLC;
    write_u16_le(&bus->last_rx.data[0], (uint16_t)pack_temperature_tenths_c);
    write_u16_le(&bus->last_rx.data[2], pack_voltage_deci_v);
    bus->last_rx.data[4] = (uint8_t)vehicle_mode;
    bus->last_rx.data[5] = 0u;
    bus->last_rx.data[6] = 0u;
    bus->last_rx.data[7] = 0u;
    bus->last_rx.timestamp_ms = timestamp_ms;
    bus->has_bms_status = true;
}

bool can_bus_sim_read_fresh_bms_status(const CanBusSim *bus,
                                       uint32_t now_ms,
                                       uint32_t timeout_ms,
                                       BmsThermalStatus *status)
{
    uint32_t age_ms;

    if ((bus == NULL) || (status == NULL) || !bus->has_bms_status ||
        (bus->last_rx.id != CAN_ID_BMS_THERMAL_STATUS) || (bus->last_rx.dlc != CAN_FRAME_DLC)) {
        return false;
    }

    age_ms = now_ms - bus->last_rx.timestamp_ms;
    if (age_ms >= timeout_ms) {
        return false;
    }

    status->pack_temperature_tenths_c = (int16_t)read_u16_le(&bus->last_rx.data[0]);
    status->pack_voltage_deci_v = read_u16_le(&bus->last_rx.data[2]);
    status->vehicle_mode = (bus->last_rx.data[4] == (uint8_t)VEHICLE_MODE_CHARGE)
                               ? VEHICLE_MODE_CHARGE
                               : VEHICLE_MODE_DRIVE;
    status->timestamp_ms = bus->last_rx.timestamp_ms;
    return true;
}

void can_bus_sim_publish_thermal_command(CanBusSim *bus,
                                         uint8_t output_mask,
                                         uint8_t torque_limit_percent,
                                         uint8_t charge_limit_percent,
                                         uint8_t ecu_state,
                                         uint8_t dtc_flags,
                                         uint32_t timestamp_ms)
{
    if (bus == NULL) {
        return;
    }

    bus->last_tx.id = CAN_ID_THERMAL_COMMAND;
    bus->last_tx.dlc = CAN_FRAME_DLC;
    bus->last_tx.data[0] = output_mask;
    bus->last_tx.data[1] = torque_limit_percent;
    bus->last_tx.data[2] = charge_limit_percent;
    bus->last_tx.data[3] = ecu_state;
    bus->last_tx.data[4] = dtc_flags;
    bus->last_tx.data[5] = (uint8_t)(bus->tx_count & 0xFFu);
    bus->last_tx.data[6] = 0u;
    bus->last_tx.data[7] = 0u;
    bus->last_tx.timestamp_ms = timestamp_ms;
    bus->tx_count += 1u;
}
