// Speed-density fuelling with wideband closed loop and VE learning (src/fuel.cpp).
//
//   air/cyl  = VE(rpm, MAP) * MAP * Vcyl / (R * T_intake)
//   fuel/cyl = air / AFR_target(MAP) * trim
//   pulse    = fuel / squirts_per_cycle / injector_flow + dead_time(battery)
//
// trim: integral controller on wideband error (lean -> more fuel), limited to +-trim_limit.
// learn: a slow fraction of the trim is moved into the VE cells being used, so trim relaxes to 1.
#pragma once
#include <cstdint>
#include "table.hpp"

namespace fuel
{
    constexpr std::size_t VE_RPM = 16, VE_MAP = 16, AFR_N = 8, DEAD_N = 4;

    // Plain storage (becomes a TunerStudio page). Scaled integers, little-endian.
    // Ordered so there is no padding: views can point straight into it.
    struct Tables
    {
        uint16_t rpm_bins[VE_RPM];          // rpm
        uint16_t map_bins[VE_MAP];          // kPa x10
        uint16_t ve[VE_MAP * VE_RPM];       // % x10, rows = MAP, columns = rpm
        uint16_t afr_map_bins[AFR_N];       // kPa x10
        uint16_t dead_volt_bins[DEAD_N];    // battery mV
        uint16_t dead_us[DEAD_N];           // injector dead time, us
        uint16_t inj_flow_mg_s;             // mg/s per injector at rail pressure
        uint16_t displacement_cc;
        uint16_t crank_pw_us;               // fixed pulse below crank_rpm
        uint16_t crank_rpm;
        uint16_t cl_min_rpm;
        uint8_t  afr_target[AFR_N];         // AFR x10
        uint8_t  squirts_per_cycle;         // 2 = paired, once per revolution (current scheduler)
        uint8_t  cl_enable;                 // bit0 closed loop, bit1 VE learning
        uint8_t  trim_limit_pct;            // +-%
        uint8_t  cl_gain_x100;              // integral gain, 1/s x100
        uint8_t  learn_rate_x100;           // fraction of trim moved into VE per second x100
        uint8_t  cl_min_clt;                // C + 40
    };
    static_assert(sizeof(Tables) == 2 * (VE_RPM + VE_MAP + VE_MAP * VE_RPM + AFR_N + 2 * DEAD_N + 5) + AFR_N + 6,
                  "Tables must have no padding");

    // Views over the storage: these are what the fuel code uses.
    struct Views
    {
        tbl::curve<uint16_t, VE_RPM>         rpm_bins;
        tbl::curve<uint16_t, VE_MAP>         map_bins;
        tbl::map2d<uint16_t, VE_MAP, VE_RPM> ve;
        tbl::curve<uint16_t, AFR_N>          afr_map_bins;
        tbl::curve<uint8_t,  AFR_N>          afr_target;
        tbl::curve<uint16_t, DEAD_N>         dead_volt_bins;
        tbl::curve<uint16_t, DEAD_N>         dead_us;
        explicit Views(Tables& t)
            : rpm_bins(t.rpm_bins), map_bins(t.map_bins), ve(t.ve), afr_map_bins(t.afr_map_bins),
              afr_target(t.afr_target), dead_volt_bins(t.dead_volt_bins), dead_us(t.dead_us) {}
    };

    struct Status
    {
        volatile uint16_t pw_us;            // what the scheduler uses
        volatile uint16_t ve_x10;
        volatile uint16_t afr_target_x10;
        volatile int16_t  trim_x1000;       // 1000 = no correction
        volatile uint16_t air_mg_x100;      // air per cylinder, mg x100
        volatile bool     closed_loop;
        volatile bool     learning;
        volatile bool     cranking;
        volatile bool     dfco;             // overrun fuel cut active
    };

    extern Tables tables;
    extern Status status;

    // Read-only "how well tuned is each cell" view, exposed as TunerStudio page 3 and to the sim.
    // Self-contained (its own axis copies) so its table editor needs no cross-page bin references.
    // status: 0 = untouched/default, 100 = enough closed-loop dwell with AFR on target.
    struct __attribute__((packed)) StatusPage
    {
        uint16_t rpm_bins[VE_RPM];          // copy of tables.rpm_bins
        uint16_t map_bins[VE_MAP];          // copy of tables.map_bins (kPa x10)
        uint8_t  tuned[VE_MAP * VE_RPM];     // %, rows = MAP, cols = rpm
    };
    extern StatusPage status_page;

    void load_defaults();
    void update(float dt_s);                // call periodically from a task (e.g. every 10 ms)
    uint16_t pulse_us();                    // ISR-safe
}
