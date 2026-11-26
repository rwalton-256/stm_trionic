// Event-driven peripheral model: one simulated microsecond clock drives TIM1/3/4/7
// (all run at 1 MHz in the firmware). FreeRTOS time is not involved.
#pragma once
#include <cstdint>
#include <vector>

namespace sim
{
    struct Edge
    {
        uint64_t t_us;
        char     timer;      // '1' = spark (TIM1), '4' = injector (TIM4)
        uint8_t  channel;    // 0..3 = cylinder 1..4
        bool     level;      // true = MOSFET on (injector open / trigger line grounded)
    };

    void     reset();
    uint64_t now();
    void     advance_to(uint64_t t_us);      // runs every compare match / wrap on the way
    void     crank_edge();                   // a crank tooth edge on PA6 at now()
    std::vector<Edge>& edges();              // output edges seen so far

    // Test hooks into stubs (power / tune) so outputs are allowed to fire.
    void set_ignition(bool on);
}
