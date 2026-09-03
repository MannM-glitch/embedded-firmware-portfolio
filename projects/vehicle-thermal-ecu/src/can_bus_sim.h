#ifndef CAN_BUS_SIM_H
#define CAN_BUS_SIM_H

#include <stdbool.h>
#include <stdint.h>

enum {
    CAN_FRAME_DLC = 8u,
    CAN_ID_BMS_THERMAL_STATUS = 0x18FF50E5u,
    CAN_ID_THERMAL_COMMAND = 0x18FF51E5u
};

typedef enum {
    VEHICLE_MODE_DRIVE = 0u,
    VEHICLE_MODE_CHARGE = 1u
} VehicleMode;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[CAN_FRAME_DLC];
    uint32_t timestamp_ms;
} CanFrame;

typedef struct {
    int16_t pack_temperature_tenths_c;
    uint16_t pack_voltage_deci_v;
    VehicleMode vehicle_mode;
    uint32_t timestamp_ms;
} BmsThermalStatus;

typedef struct {
    bool has_bms_status;
    CanFrame last_rx;
    CanFrame last_tx;
    uint32_t tx_count;
} CanBusSim;

void can_bus_sim_init(CanBusSim *bus);
void can_bus_sim_inject_bms_status(CanBusSim *bus,
                                   int16_t pack_temperature_tenths_c,
                                   uint16_t pack_voltage_deci_v,
                                   VehicleMode vehicle_mode,
                                   uint32_t timestamp_ms);
bool can_bus_sim_read_fresh_bms_status(const CanBusSim *bus,
                                       uint32_t now_ms,
                                       uint32_t timeout_ms,
                                       BmsThermalStatus *status);
void can_bus_sim_publish_thermal_command(CanBusSim *bus,
                                         uint8_t output_mask,
                                         uint8_t torque_limit_percent,
                                         uint8_t charge_limit_percent,
                                         uint8_t ecu_state,
                                         uint8_t dtc_flags,
                                         uint32_t timestamp_ms);

#endif
