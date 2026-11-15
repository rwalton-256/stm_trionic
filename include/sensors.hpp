#pragma once
#include <cstdint>

// Slow analog sensors, sampled by one low-priority task (src/sensors.cpp).
namespace sensors
{
    // External inputs on the shield, in scan order.
    enum Input : uint8_t { IAT, CLT, TPS, MAP, BATTERY, WIDEBAND, INPUT_COUNT };

    struct Readings
    {
        volatile uint16_t pin_mv[INPUT_COUNT]; // voltage at each MCU pin, mV
        volatile uint16_t vdda_mv;             // measured analog supply (0 until first reading)
        volatile int16_t  mcu_temp_deci_c;     // STM32 die temperature, C x10
        volatile uint16_t battery_mv;          // battery voltage, mV
        volatile int16_t  iat_deci_c;          // intake air temperature, C x10
        volatile int16_t  clt_deci_c;          // coolant temperature, C x10
        volatile uint16_t tps_sensor_mv;       // TPS voltage at the sensor (before divider)
        volatile uint16_t tps_x10;             // throttle, % x10
        volatile uint16_t map_kpa_x10;         // manifold absolute pressure, kPa x10
        volatile uint16_t afr_x10;             // wideband AFR x10
    };
    extern Readings readings;
}