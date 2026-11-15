#pragma once
#include <cstdint>

// Ignition (+15) handling, main and fuel pump relays, and sleep (src/power.cpp).
//
//   +15 sense:   PG12 (Arduino D8)  via R52/R54 divider from T5 pin 60
//   Main relay:  PG7  (Arduino D6)  -> Q14 -> T5 pin 31   (high = relay on)
//   Fuel relay:  PG8  (Arduino D7)  -> Q15 -> T5 pin 56   (high = relay on)
//
// Injectors and spark only fire with ignition on AND the main relay on.
// With ignition off the ECU stays awake (all outputs off) while TunerStudio or a firmware
// update is connected, then goes to STOP mode. +15 wakes it, which restarts the firmware.
namespace power
{
    enum State : uint8_t { BOOT = 0, IGN_ON = 1, IGN_OFF = 2, SLEEPING = 3 };

    enum Flags : uint8_t
    {
        F_IGN_INPUT  = 1,    // raw +15 input
        F_MAIN_RELAY = 2,
        F_FUEL_PUMP  = 4,
        F_BENCH_IGN  = 8,    // bench override active
        F_HELD_AWAKE = 16,   // ignition off but kept awake (TunerStudio / update / boot grace)
    };

    struct Status
    {
        volatile uint8_t  state;
        volatile uint8_t  flags;
        volatile uint16_t sleep_in_ds;   // tenths of a second until sleep (0 = not counting)
    };
    extern Status status;

    extern volatile bool client_connected;   // set by ts_link while TunerStudio is connected
    extern volatile bool update_in_progress; // set by ota while receiving firmware

    bool outputs_allowed();                  // ISR-safe
}