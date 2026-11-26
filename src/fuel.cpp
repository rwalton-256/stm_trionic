// fuel.cpp - see fuel.hpp
#include <cmath>
#include "fuel.hpp"
#include "crank.hpp"
#include "sensors.hpp"
#include "tune.hpp"

namespace fuel
{
    Tables tables{};
    Status status{};
    StatusPage status_page{};

    // Per-cell tuning confidence 0..1 (shown as 0..100%). Rises with weighted closed-loop dwell
    // while AFR sits on target, and is pulled back down while AFR is off, so a cell only goes
    // green once it has been held with the mixture actually on target.
    static float ve_conf[VE_MAP * VE_RPM];
    constexpr float CONF_UP   = 0.9f;         // per weighted-second toward 1 when AFR on target
    constexpr float CONF_DN   = 0.5f;         // per weighted-second toward 0 when AFR off
    constexpr float QUAL_BAND = 0.08f;        // AFR fractional error (~8%) that scores quality 0

    static float trim = 1.0f;
    void load_defaults();
    namespace { struct DefaultsAtStartup { DefaultsAtStartup() { load_defaults(); } } defaults_at_startup; }
    static float learn_residual[VE_MAP * VE_RPM];   // sub-count VE changes not yet written to the table
    constexpr float R_AIR = 287.05f;

    void load_defaults()
    {
        Tables& t = tables;
        const uint16_t rpm[VE_RPM] = { 500, 800, 1100, 1400, 1700, 2000, 2400, 2800,
                                       3200, 3600, 4000, 4500, 5000, 5500, 6000, 6500 };
        for (std::size_t i = 0; i < VE_RPM; i++) t.rpm_bins[i] = rpm[i];
        for (std::size_t i = 0; i < VE_MAP; i++) t.map_bins[i] = uint16_t(200 + i * 60);   // 20..110 kPa
        Views v(t);
        for (std::size_t r = 0; r < VE_MAP; r++)
            for (std::size_t c = 0; c < VE_RPM; c++)
                tbl::cell(v.ve, r, c) = 750;                                              // flat 75 %
        const uint16_t afr_map[AFR_N] = { 200, 400, 600, 800, 900, 1000, 1050, 1100 };
        const uint8_t  afr[AFR_N]     = { 147, 147, 147, 147, 140, 130, 125, 125 };
        for (std::size_t i = 0; i < AFR_N; i++) { t.afr_map_bins[i] = afr_map[i]; t.afr_target[i] = afr[i]; }
        const uint16_t volts[DEAD_N] = { 10000, 12000, 14000, 16000 };
        const uint16_t dead[DEAD_N]  = { 1200, 800, 650, 550 };
        for (std::size_t i = 0; i < DEAD_N; i++) { t.dead_volt_bins[i] = volts[i]; t.dead_us[i] = dead[i]; }
        t.inj_flow_mg_s     = 4070;         // ~330 cc/min petrol (B204L stock)
        t.displacement_cc   = 1985;
        t.crank_pw_us       = 6000;
        t.crank_rpm         = 400;
        t.squirts_per_cycle = 2;
        t.cl_enable         = 3;
        t.trim_limit_pct    = 20;
        t.cl_gain_x100      = 50;           // 0.5 /s: slow vs. the ~0.1-0.2 s wideband delay
        t.learn_rate_x100   = 20;           // 0.2 /s
        t.cl_min_rpm        = 600;
        t.cl_min_clt        = 40 + 40;      // 40 C
        trim = 1.0f;
        for (auto& r : learn_residual) r = 0.0f;
        // reset tuning status and seed the status page's own axis copies
        for (auto& c : ve_conf) c = 0.0f;
        for (auto& s : status_page.tuned) s = 0;
        for (std::size_t i = 0; i < VE_RPM; i++) status_page.rpm_bins[i] = t.rpm_bins[i];
        for (std::size_t i = 0; i < VE_MAP; i++) status_page.map_bins[i] = t.map_bins[i];
    }

    // Move a fraction of the trim into the four VE cells around (map, rpm), by bilinear weight.
    static void learn(const Views& v, float map_x10, float rpm, float amount)
    {
        tbl::Pos py = tbl::locate(v.map_bins, map_x10), px = tbl::locate(v.rpm_bins, rpm);
        const float w[4] = { (1 - py.f) * (1 - px.f), (1 - py.f) * px.f, py.f * (1 - px.f), py.f * px.f };
        const std::size_t r[4] = { py.i, py.i, py.i + 1, py.i + 1 }, c[4] = { px.i, px.i + 1, px.i, px.i + 1 };
        for (int k = 0; k < 4; k++)
        {
            auto&  cell = tbl::cell(v.ve, r[k], c[k]);
            float& res  = learn_residual[r[k] * VE_RPM + c[k]];
            res += float(cell) * amount * w[k];                             // change in counts
            int whole = int(res);                                           // write whole counts only
            if (whole != 0)
            {
                int nv = int(cell) + whole;
                cell = uint16_t(nv < 100 ? 100 : nv > 1500 ? 1500 : nv);    // 10..150 %
                res -= float(whole);
            }
        }
    }

    // Raise each visited cell's tuning status: weighted dwell toward "enough", and an AFR-quality
    // EMA. status% = dwell-fraction * quality, so a cell is only "tuned" after real closed-loop
    // time there with AFR on target. Called only while closed loop is active (else frozen).
    static void update_status(const Views& v, float map_x10, float rpm, float afr_meas, float afr_tgt, float dt)
    {
        tbl::Pos py = tbl::locate(v.map_bins, map_x10), px = tbl::locate(v.rpm_bins, rpm);
        const float w[4] = { (1 - py.f) * (1 - px.f), (1 - py.f) * px.f, py.f * (1 - px.f), py.f * px.f };
        const std::size_t r[4] = { py.i, py.i, py.i + 1, py.i + 1 }, c[4] = { px.i, px.i + 1, px.i, px.i + 1 };
        float err = afr_tgt > 0 ? std::fabs(afr_meas / afr_tgt - 1.0f) : 1.0f;
        float quality = 1.0f - err / QUAL_BAND;
        quality = quality < 0 ? 0 : quality > 1 ? 1 : quality;
        for (int k = 0; k < 4; k++)
        {
            if (w[k] <= 0.0f) continue;
            std::size_t i = r[k] * VE_RPM + c[k];
            float& conf = ve_conf[i];
            conf += w[k] * dt * (CONF_UP * quality * (1.0f - conf) - CONF_DN * (1.0f - quality) * conf);
            conf = conf < 0 ? 0 : conf > 1 ? 1 : conf;
            status_page.tuned[i] = uint8_t(conf * 100.0f + 0.5f);
        }
    }

    void update(float dt)
    {
        const Tables& t = tables;
        Views v(tables);
        const auto& s = sensors::readings;
        for (std::size_t i = 0; i < VE_RPM; i++) status_page.rpm_bins[i] = t.rpm_bins[i];
        for (std::size_t i = 0; i < VE_MAP; i++) status_page.map_bins[i] = t.map_bins[i];

        float rpm  = float(crank::status.rpm);
        float map  = float(s.map_kpa_x10);                         // kPa x10
        float iat  = float(s.iat_deci_c) / 10.0f + 273.15f;
        float batt = float(s.battery_mv);

        float ve_pct  = tbl::interp(v.ve, v.map_bins, v.rpm_bins, map, rpm, 0.1f);
        float afr_tgt = tbl::interp(v.afr_target, v.afr_map_bins, map, 0.1f);
        float dead_us = tbl::interp(v.dead_us, v.dead_volt_bins, batt);

        float vcyl  = float(t.displacement_cc) * 1e-6f / 4.0f;     // m^3
        float air   = ve_pct / 100.0f * map * 100.0f * vcyl / (R_AIR * iat);   // kg
        bool  crank = crank::status.synced && rpm < float(t.crank_rpm);
        bool  warm  = (s.clt_deci_c / 10 + 40) >= int(t.cl_min_clt);

        // ---- overrun fuel cut (DFCO) ----
        // Cut injectors when coasting: throttle closed, warm, synced, rpm above the cut line;
        // restore below the (lower) resume line. Closed throttle + high rpm is the standard proxy
        // for "the wheels are driving the engine". Hysteresis keeps it from chattering.
        const TunePage1& tp = tune_page1;
        static bool dfco = false;
        bool closed = float(s.tps_x10) / 10.0f <= float(tp.dfco_tps_pct);
        bool want_cut = tp.dfco_enable && !crank && warm && closed;
        dfco = want_cut && (dfco ? rpm > float(tp.dfco_resume_rpm) : rpm > float(tp.dfco_cut_rpm));

        // ---- closed loop (frozen while fuel is cut: nothing to measure, don't wind up) ----
        float afr_meas = float(s.afr_x10) / 10.0f;
        bool  wb_ok = afr_meas > 9.0f && afr_meas < 19.0f;
        bool  cl = (t.cl_enable & 1) && !crank && !dfco && wb_ok && rpm >= float(t.cl_min_rpm) && warm;
        float lim = float(t.trim_limit_pct) / 100.0f;
        if (cl)
        {
            float err = afr_meas / afr_tgt - 1.0f;                 // + = lean
            trim += float(t.cl_gain_x100) / 100.0f * err * dt;
            trim = std::fmin(std::fmax(trim, 1.0f - lim), 1.0f + lim);
        }
        bool learning = cl && (t.cl_enable & 2);
        if (learning)
        {
            float amount = float(t.learn_rate_x100) / 100.0f * (trim - 1.0f) * dt;
            learn(v, map, rpm, amount);
            trim /= (1.0f + amount);                               // VE took it, trim gives it back
        }
        if (cl)
            update_status(v, map, rpm, afr_meas, afr_tgt, dt);    // tuning-status heatmap (page 3)

        float pw;
        if (crank)
            pw = float(t.crank_pw_us);
        else if (dfco)
            pw = 0.0f;                                             // injectors closed on overrun
        else
        {
            float fuel_kg = air / afr_tgt * trim;
            float flow    = float(t.inj_flow_mg_s) * 1e-6f;        // kg/s
            pw = fuel_kg / float(t.squirts_per_cycle ? t.squirts_per_cycle : 1) / flow * 1e6f + dead_us;
        }

        status.pw_us          = uint16_t(std::fmin(std::fmax(pw, 0.0f), 60000.0f));
        status.ve_x10         = uint16_t(ve_pct * 10.0f);
        status.afr_target_x10 = uint16_t(afr_tgt * 10.0f);
        status.trim_x1000     = int16_t(trim * 1000.0f);
        status.air_mg_x100     = uint16_t(std::fmin(air * 1e8f, 65535.0f));
        status.closed_loop    = cl;
        status.learning       = learning;
        status.cranking       = crank;
        status.dfco           = dfco;
    }

    uint16_t pulse_us() { return status.pw_us; }
}
