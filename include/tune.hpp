#pragma once
#include <cstdint>

// Tune page 1. Layout MUST match [Constants] page = 1 in tuning/stm_trionic.ini.
struct __attribute__((packed)) TunePage1
{
    uint8_t  tooth_positions;   //  0: tooth positions incl. missing (e.g. 60)
    uint8_t  missing_teeth;     //  1: number of missing teeth (e.g. 2)
    uint16_t batt_ratio_x1000;  //  2: battery divider (5120 = 41.2k over 10k)
    uint16_t in5v_ratio_x1000;  //  4: 5 V sensor dividers (1500 = 10k over 20k)
    uint16_t therm_pullup_ohm;  //  6: IAT/CLT pull-up to 3.3 V (R40, R47)
    uint16_t therm_r25_ohm;     //  8: NTC resistance at 25 C
    uint16_t therm_beta;        // 10: NTC beta, K
    uint16_t tps_min_mv;        // 12: TPS sensor voltage, closed
    uint16_t tps_max_mv;        // 14: TPS sensor voltage, wide open
    uint16_t map_kpa0_x10;      // 16: MAP kPa at 0 V sensor output
    uint16_t map_kpa5_x10;      // 18: MAP kPa at 5 V sensor output
    uint8_t  afr0_x10;          // 20: wideband AFR at 0 V
    uint8_t  afr5_x10;          // 21: wideband AFR at 5 V
    uint16_t trigger_angle;     // 22: tooth 1 (first after the gap), deg BTDC cylinder 1
    int8_t   advance;           // 24: fixed spark advance, deg BTDC
    uint16_t spark_pulse_us;    // 25: DI trigger low time (>= 500 us)
    uint16_t inj_pw_us;         // 27: fixed injector pulse width
    uint16_t inj_angle;         // 29: injection start, deg BTDC of each pair
    uint16_t rev_limit;         // 31: rpm, hard cut
    uint8_t  outputs_enable;    // 33: bit0 injectors, bit1 spark (0 = nothing fires)
    uint16_t sim_rpm;           // 34: internal crank simulator, 0 = off (use the real sensor)
    uint8_t  bench_ign;         // 36: bench only: act as if +15 were on (resets at power-up)
    uint8_t  fuel_mode;         // 37: 0 = fixed inj_pw_us, 1 = speed-density (fuel.cpp)
    uint8_t  dfco_enable;       // 38: 1 = cut injectors on overrun (closed throttle, high rpm)
    uint8_t  dfco_tps_pct;      // 39: closed-throttle threshold, %
    uint16_t dfco_cut_rpm;      // 40: cut fuel above this rpm
    uint16_t dfco_resume_rpm;   // 42: restore fuel below this rpm (hysteresis; < cut)
    uint8_t  reserved[20];      // 44..63
};
static_assert(sizeof(TunePage1) == 64, "page 1 must be 64 bytes");

// Live data block. Layout MUST match [OutputChannels] in the ini.
struct __attribute__((packed)) OutputChannels
{
    uint8_t  secl;             //  0: seconds counter (wraps), TS uses it to detect resets
    uint16_t rpm;              //  1
    uint8_t  tooth;            //  3
    uint8_t  synced;           //  4
    uint16_t sync_losses;      //  5
    uint16_t tooth_period_us;  //  7
    uint32_t revs;             //  9
    uint8_t  mcu_temp;         // 13: C + 40
    uint8_t  battery_x10;      // 14: V x10
    uint8_t  iat;              // 15: C + 40
    uint8_t  clt;              // 16: C + 40
    uint8_t  tps_x2;           // 17: % x2
    uint16_t map_x10;          // 18: kPa x10
    uint8_t  afr_x10;          // 20: AFR x10
    uint8_t  reserved1;        // 21
    uint16_t tps_mv;           // 22: TPS sensor-side voltage, mV (for calibration)
    uint16_t stale_edges;      // 24: crank diagnostics
    uint16_t gap_mismatch;     // 26
    uint16_t overcount;        // 28
    uint16_t stopped;          // 30
    int8_t   advance;          // 32: deg BTDC
    uint8_t  out_flags;        // 33: bit0 fuel, bit1 spark, bit2 rev limit, bit3 simulator, bit4 dfco
    uint16_t pw_us;            // 34: injector pulse width
    uint16_t sparks;           // 36
    uint16_t injections;       // 38
    uint16_t spark_skipped;    // 40
    uint16_t overlaps;         // 42
    uint16_t spark_pulses;     // 44: completed in hardware
    uint16_t inj_pulses;       // 46
    uint8_t  sd_state;         // 48: 0 starting, 1 ready, 2 no card, 3 mount failed, 4 I/O error
    uint8_t  sd_error;         // 49: last FatFs result code
    uint16_t sd_size_gb_x10;   // 50
    uint16_t sd_free_gb_x10;   // 52
    uint16_t sd_write_errors;  // 54
    uint8_t  power_state;      // 56: 0 boot, 1 ignition on, 2 ignition off, 3 sleeping
    uint8_t  power_flags;      // 57: see power::Flags
    uint16_t sleep_in_ds;      // 58: tenths of a second until sleep
    uint8_t  settings_source;  // 60: 0 defaults, 1 tune_a.bin, 2 tune_b.bin
    uint8_t  settings_dirty;   // 61: unsaved changes
    uint16_t settings_saves;   // 62: saves since boot
};
static_assert(sizeof(OutputChannels) == 64, "ochBlockSize must be 64");

extern TunePage1 tune_page1;
