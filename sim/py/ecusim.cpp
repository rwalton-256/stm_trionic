// ecusim: the stm_trionic firmware on the real FreeRTOS scheduler, in simulated time.
//
// C++ owns the clock: advance_to(t) runs the timer model up to t and delivers one FreeRTOS tick
// per simulated millisecond, waiting each time until every firmware task is blocked again.
// Python only exchanges engine I/O: crank edges and sensor values in, injector/spark edges and
// status out. Running tasks: ts_link (TunerStudio on 127.0.0.1:29001), fuel, power, SD card
// (settings on a virtual card). One ECU per process: start() boots it once.
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "periph.hpp"
#include "board.hpp"
#include "lockstep.hpp"
#include "crank.hpp"
#include "outputs.hpp"
#include "fuel.hpp"
#include "power.hpp"
#include "sensors.hpp"
#include "settings.hpp"
#include "tune.hpp"
#include "stm32f4xx_hal.h"

namespace py = pybind11;
static size_t g_edge_cursor = 0;
extern uint16_t sim_ts_port;     // net/sockets_shim.cpp

static void need_started()
{
    if (!sim::rtos::started()) throw std::runtime_error("call ecusim.start() first");
}

static void advance_to(uint64_t t_us)
{
    need_started();
    py::gil_scoped_release nogil;
    for (;;)
    {
        uint64_t next_tick = (uint64_t(sim::rtos::ticks()) + 1) * 1000;
        if (next_tick > t_us) break;
        sim::advance_to(next_tick);
        sim::rtos::tick();
    }
    sim::advance_to(t_us);
}

static void set_ignition(bool on)
{
    if (on) sim_gpiog.IDR |= GPIO_PIN_12; else sim_gpiog.IDR &= ~uint32_t(GPIO_PIN_12);
}

PYBIND11_MODULE(ecusim, m)
{
    m.doc() = "stm_trionic firmware on the FreeRTOS scheduler, lockstepped to a simulated clock";

    // ---- lifecycle and time ----
    m.def("start", [](std::map<std::string, py::bytes> card, bool ignition) {
        if (sim::rtos::started()) throw std::runtime_error("the ECU is already running (one per process)");
        for (auto& [k, v] : card) { std::string s = v; sim::board::card.files[k].assign(s.begin(), s.end()); }
        sim::reset();
        set_ignition(ignition);
        sim::rtos::start();
    }, py::arg("card") = std::map<std::string, py::bytes>{}, py::arg("ignition") = true,
       "Boot the ECU. card: files on the virtual SD card at power-up {name: bytes}");
    m.def("started", &sim::rtos::started);
    m.def("ts_port", [] { return sim_ts_port; }, "TCP port TunerStudio can connect to (0 until ts_link is up)");
    m.def("now_us", &sim::now);
    m.def("advance_to", &advance_to, py::arg("t_us"), "Run the ECU up to t_us (timers + 1 ms RTOS ticks)");

    // ---- engine I/O ----
    m.def("crank_edge", [] { need_started(); sim::crank_edge(); }, "A crank tooth edge on PA6 now");
    m.def("set_ignition", &set_ignition, "+15 input (PG12)");
    m.def("take_edges", [] {
        auto& e = sim::edges();
        py::list out;
        for (; g_edge_cursor < e.size(); g_edge_cursor++)
        {
            const auto& x = e[g_edge_cursor];
            out.append(py::make_tuple(x.t_us, x.timer == '1' ? "spark" : "inj", int(x.channel), x.level));
        }
        return out;
    }, "New output edges since the last call: (t_us, 'spark'|'inj', cylinder 0..3, on)");
    m.def("set_sensors", [](double map_kpa, double iat_c, double clt_c, double battery_v, double afr,
                            double throttle) {
        auto& s = sensors::readings;
        s.map_kpa_x10 = uint16_t(map_kpa * 10 + 0.5);
        s.iat_deci_c  = int16_t(iat_c * 10);
        s.clt_deci_c  = int16_t(clt_c * 10);
        s.battery_mv  = uint16_t(battery_v * 1000);
        s.afr_x10     = uint16_t(afr * 10 + 0.5);
        // Throttle as the TPS sensor: percent, and a sensor voltage ~0.5..4.5 V so the ECU's
        // TPS % (from its own calibration) and the TPS Volts gauge both move in TunerStudio.
        double pct = throttle * 100.0;
        if (pct < 0) pct = 0; else if (pct > 100) pct = 100;
        s.tps_x10       = uint16_t(pct * 10 + 0.5);
        s.tps_sensor_mv = uint16_t(500 + 4000 * throttle + 0.5);
    }, py::arg("map_kpa"), py::arg("iat_c"), py::arg("clt_c"), py::arg("battery_v"), py::arg("afr"),
       py::arg("throttle") = 0.0);
    m.def("relays", [] {
        return py::dict(py::arg("main") = bool(sim_gpiog.ODR & GPIO_PIN_7),
                        py::arg("fuel_pump") = bool(sim_gpiog.ODR & GPIO_PIN_8));
    });

    // ---- tune (what TunerStudio would write) ----
    m.def("tune", [] {
        const auto& t = tune_page1;
        return py::dict(py::arg("advance") = int(t.advance), py::arg("inj_angle") = int(t.inj_angle),
                        py::arg("fuel_mode") = int(t.fuel_mode), py::arg("rev_limit") = int(t.rev_limit),
                        py::arg("trigger_angle") = int(t.trigger_angle), py::arg("inj_pw_us") = int(t.inj_pw_us),
                        py::arg("outputs_enable") = int(t.outputs_enable), py::arg("spark_pulse_us") = int(t.spark_pulse_us));
    });
    m.def("set_tune", [](py::kwargs kw) {
        auto& t = tune_page1;
        for (auto item : kw)
        {
            auto k = item.first.cast<std::string>(); int v = item.second.cast<int>();
            if      (k == "advance")        t.advance = int8_t(v);
            else if (k == "inj_angle")      t.inj_angle = uint16_t(v);
            else if (k == "fuel_mode")      t.fuel_mode = uint8_t(v);
            else if (k == "rev_limit")      t.rev_limit = uint16_t(v);
            else if (k == "trigger_angle")  t.trigger_angle = uint16_t(v);
            else if (k == "inj_pw_us")      t.inj_pw_us = uint16_t(v);
            else if (k == "outputs_enable") t.outputs_enable = uint8_t(v);
            else if (k == "spark_pulse_us") t.spark_pulse_us = uint16_t(v);
            else if (k == "dfco_enable")     t.dfco_enable = uint8_t(v);
            else if (k == "dfco_tps_pct")    t.dfco_tps_pct = uint8_t(v);
            else if (k == "dfco_cut_rpm")    t.dfco_cut_rpm = uint16_t(v);
            else if (k == "dfco_resume_rpm") t.dfco_resume_rpm = uint16_t(v);
            else throw py::key_error(k);
        }
    });
    m.def("set_closed_loop", [](bool cl, bool learn) {
        fuel::tables.cl_enable = uint8_t((cl ? 1 : 0) | (learn ? 2 : 0));
    }, py::arg("closed_loop"), py::arg("learn"));
    m.def("ve_table", [] {
        fuel::Views v(fuel::tables);
        std::vector<std::vector<double>> out(fuel::VE_MAP, std::vector<double>(fuel::VE_RPM));
        for (size_t r = 0; r < fuel::VE_MAP; r++)
            for (size_t c = 0; c < fuel::VE_RPM; c++) out[r][c] = tbl::cell(v.ve, r, c) / 10.0;
        return out;
    }, "VE table in %, rows = MAP bins, columns = rpm bins");
    m.def("set_ve", [](size_t row, size_t col, double pct) {
        fuel::Views v(fuel::tables);
        tbl::cell(v.ve, row, col) = uint16_t(pct * 10 + 0.5);
    }, py::arg("row"), py::arg("col"), py::arg("pct"));
    m.def("ve_status", [] {
        std::vector<std::vector<int>> out(fuel::VE_MAP, std::vector<int>(fuel::VE_RPM));
        for (size_t r = 0; r < fuel::VE_MAP; r++)
            for (size_t c = 0; c < fuel::VE_RPM; c++)
                out[r][c] = fuel::status_page.tuned[r * fuel::VE_RPM + c];
        return out;
    }, "VE tuning status %, rows = MAP bins, columns = rpm bins");
    m.def("ve_axes", [] {
        std::vector<int> rpm(fuel::tables.rpm_bins, fuel::tables.rpm_bins + fuel::VE_RPM);
        std::vector<double> map; for (auto x : fuel::tables.map_bins) map.push_back(x / 10.0);
        return py::make_tuple(rpm, map);
    });

    // ---- status ----
    m.def("crank_status", [] {
        const auto& c = crank::status;
        return py::dict(py::arg("rpm") = int(c.rpm), py::arg("synced") = bool(c.synced),
                        py::arg("tooth") = int(c.tooth), py::arg("sync_losses") = int(c.sync_losses));
    });
    m.def("fuel_status", [] {
        const auto& f = fuel::status;
        return py::dict(py::arg("pw_us") = int(f.pw_us), py::arg("ve") = f.ve_x10 / 10.0,
                        py::arg("afr_target") = f.afr_target_x10 / 10.0, py::arg("trim") = f.trim_x1000 / 1000.0,
                        py::arg("closed_loop") = bool(f.closed_loop), py::arg("learning") = bool(f.learning),
                        py::arg("cranking") = bool(f.cranking), py::arg("dfco") = bool(f.dfco));
    });
    m.def("output_status", [] {
        const auto& o = outputs::status;
        return py::dict(py::arg("sparks") = int(o.sparks), py::arg("injections") = int(o.injections),
                        py::arg("overlaps") = int(o.overlaps), py::arg("spark_skipped") = int(o.spark_skipped),
                        py::arg("rev_limited") = bool(o.rev_limited));
    });
    m.def("power_status", [] {
        static const char* names[] = { "boot", "ign_on", "ign_off", "sleeping" };
        const auto& p = power::status;
        return py::dict(py::arg("state") = names[p.state & 3], py::arg("outputs_allowed") = power::outputs_allowed(),
                        py::arg("sleep_in_s") = p.sleep_in_ds / 10.0,
                        py::arg("stopped") = sim::board::stopped, py::arg("reset") = sim::board::reset);
    });

    // ---- settings / virtual SD card ----
    m.def("burn", [] { settings::request_save(); }, "Same as TunerStudio's burn button");
    m.def("settings_status", [] {
        static const char* src[] = { "defaults", "tune_a", "tune_b" };
        const auto& s = settings::status;
        return py::dict(py::arg("source") = src[s.source % 3], py::arg("seq") = int(s.seq),
                        py::arg("dirty") = bool(s.dirty), py::arg("saves") = int(s.saves),
                        py::arg("save_errors") = int(s.save_errors));
    });
    m.def("card_files", [] {
        py::dict d;
        for (auto& [k, v] : sim::board::card.files) d[py::str(k)] = py::bytes(reinterpret_cast<const char*>(v.data()), v.size());
        return d;
    }, "Files on the virtual SD card {name: bytes}");
    m.def("card_present", [](bool p) { sim::board::card.present = p; });
    m.def("card_tear_next_write", [](size_t n) { sim::board::card.tear_next_write = n; },
          "Simulate a power cut: the next file write stops after n bytes");
}
