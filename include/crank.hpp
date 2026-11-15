#pragma once
#include <cstdint>

namespace crank
{
    // Trigger wheel. CHECK against the Trionic 5 manual (60-2 assumed).
    constexpr uint32_t TOOTH_POSITIONS = 60;
    constexpr uint32_t MISSING_TEETH   = 2;
    constexpr uint32_t REAL_TEETH      = TOOTH_POSITIONS - MISSING_TEETH;
    constexpr uint32_t TICK_HZ         = 1000000;

    struct Status
    {
        volatile uint32_t rpm;
        volatile uint32_t tooth_period;
        volatile uint8_t  tooth;        // 1 = first tooth after the gap
        volatile bool     synced;
        volatile uint32_t sync_losses;
        volatile uint32_t revs;
        // Diagnostics: why sync was dropped (counters wrap)
        volatile uint16_t stale_edges;   // tooth after a >65 ms silence, discarded
        volatile uint16_t gap_mismatch;  // gap arrived after the wrong number of teeth
        volatile uint16_t overcount;     // more teeth than the wheel has, no gap seen
        volatile uint16_t stopped;       // ~131 ms with no tooth at all
        volatile bool     simulating;    // teeth come from the internal simulator, not PA6
    };
    extern Status status;

    void init();
}