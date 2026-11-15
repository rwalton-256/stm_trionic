#pragma once
#include <cstdint>

// Injector and spark scheduling from crank angle (src/outputs.cpp).
//
//   Injectors: TIM4 CH1..4 -> PD12..PD15 -> T5 pins 3, 4, 5, 6   (high = injector open)
//   Spark:     TIM1 CH1..4 -> PE9, PE11, PE13, PE14 -> T5 pins 9..12
//              (high = MOSFET on = trigger line grounded; the DI cassette fires on that falling edge)
//
// Version 1: wasted spark (1+4, 3+2) and paired injection once per revolution,
// fixed advance and pulse width from the tune. No phase detection yet.
namespace outputs
{
    struct Status
    {
        volatile uint16_t sparks;          // spark pair events fired
        volatile uint16_t injections;      // injector pair events fired
        volatile uint16_t spark_skipped;   // skipped: DI cassette not recharged yet (< 2.5 ms)
        volatile uint16_t overlaps;        // skipped: channel still busy from the last event
        volatile uint16_t spark_pulses;    // trigger pulses actually completed in hardware (per channel)
        volatile uint16_t inj_pulses;      // injector pulses actually completed in hardware (per channel)
        volatile bool     rev_limited;
    };
    extern Status status;

    void init();                                    // call before crank::init()
    // From the crank ISR on every tooth while synced. tooth: 1..58, capture: TIM3 timestamp,
    // period: time per 6 deg slot (us), rpm: last full-revolution rpm.
    void on_tooth(uint8_t tooth, uint16_t capture, uint32_t period, uint32_t rpm);
    void stop();                                    // sync lost: everything off, now
}