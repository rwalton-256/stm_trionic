// ts_link.cpp - TunerStudio link over TCP (msEnvelope_1.0, same framing as Speeduino)
//
// Frame (both directions): [len_hi][len_lo] [payload ...] [crc32, big-endian]
// Request payload:  [cmd][canId][page][off_lo][off_hi][len_lo][len_hi][data...]
// Response payload: [return code][data...]
// Exception: the very first 'F' arrives raw (unframed) and is answered raw with "002".

#include <cstring>
#include <FreeRTOS.hpp>
#include "crank.hpp"
#include "tune.hpp"
#include "sensors.hpp"
#include "outputs.hpp"
#include "sdcard.hpp"
#include "power.hpp"

namespace
{
    constexpr uint16_t TS_PORT        = 29001;
    constexpr uint16_t BLOCKING       = 251;
    constexpr uint16_t TABLE_BLOCKING = 256;
    constexpr char     SIGNATURE[]    = "speeduino 202306-stm_trionic";   // must equal ini 'signature'

    enum : uint8_t
    {
        RC_OK = 0x00, RC_BURN_OK = 0x04,
        RC_TIMEOUT = 0x80, RC_CRC_ERR = 0x82, RC_UNKNOWN = 0x83, RC_RANGE_ERR = 0x84,
    };

    uint8_t rx[BLOCKING + 16];
    uint8_t tx[BLOCKING + 16];

    // Standard CRC-32 (poly 0xEDB88320, init/xorout 0xFFFFFFFF), same as FastCRC32::crc32
    uint32_t crc32(const uint8_t* p, size_t n)
    {
        uint32_t c = 0xFFFFFFFFu;
        while (n--)
        {
            c ^= *p++;
            for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
        return ~c;
    }

    bool recv_all(Socket_t s, uint8_t* p, size_t n)
    {
        while (n)
        {
            BaseType_t r = FreeRTOS_recv(s, p, n, 0);
            if (r <= 0) return false;              // timeout or closed
            p += r; n -= r;
        }
        return true;
    }

    bool send_all(Socket_t s, const uint8_t* p, size_t n)
    {
        while (n)
        {
            BaseType_t r = FreeRTOS_send(s, p, n, 0);
            if (r <= 0) return false;
            p += r; n -= r;
        }
        return true;
    }

    // Send the whole frame in one call: separate small sends stall on TCP delayed-ACK.
    uint8_t frame_buf[2 + sizeof(tx) + 4];

    bool send_frame(Socket_t s, const uint8_t* payload, uint16_t len)
    {
        if (len > sizeof(tx)) return false;
        uint32_t c = crc32(payload, len);
        frame_buf[0] = uint8_t(len >> 8);
        frame_buf[1] = uint8_t(len);
        memcpy(&frame_buf[2], payload, len);
        frame_buf[2 + len + 0] = uint8_t(c >> 24);
        frame_buf[2 + len + 1] = uint8_t(c >> 16);
        frame_buf[2 + len + 2] = uint8_t(c >> 8);
        frame_buf[2 + len + 3] = uint8_t(c);
        return send_all(s, frame_buf, size_t(len) + 6);
    }

    bool send_rc(Socket_t s, uint8_t rc) { return send_frame(s, &rc, 1); }

    uint8_t* page_ptr(uint8_t page, uint16_t& size)
    {
        if (page == 1) { size = sizeof(tune_page1); return reinterpret_cast<uint8_t*>(&tune_page1); }
        size = 0; return nullptr;
    }

    void fill_output_channels(OutputChannels& o)
    {
        o = {};
        o.secl            = uint8_t(xTaskGetTickCount() / configTICK_RATE_HZ);
        o.rpm             = uint16_t(crank::status.rpm);
        o.tooth           = crank::status.tooth;
        o.synced          = crank::status.synced ? 1 : 0;
        o.sync_losses     = uint16_t(crank::status.sync_losses);
        uint32_t tp       = crank::status.tooth_period;
        o.tooth_period_us = uint16_t(tp > 0xFFFF ? 0xFFFF : tp);
        o.revs            = crank::status.revs;
        auto u8c = [](int v) { return uint8_t(v < 0 ? 0 : v > 255 ? 255 : v); };
        const auto& r = sensors::readings;
        o.mcu_temp        = u8c(r.mcu_temp_deci_c / 10 + 40);
        o.battery_x10     = u8c((r.battery_mv + 50) / 100);
        o.iat             = u8c(r.iat_deci_c / 10 + 40);
        o.clt             = u8c(r.clt_deci_c / 10 + 40);
        o.tps_x2          = u8c((r.tps_x10 + 2) / 5);
        o.map_x10         = r.map_kpa_x10;
        o.afr_x10         = u8c(r.afr_x10);
        o.tps_mv          = r.tps_sensor_mv;
        o.stale_edges     = crank::status.stale_edges;
        o.gap_mismatch    = crank::status.gap_mismatch;
        o.overcount       = crank::status.overcount;
        o.stopped         = crank::status.stopped;
        o.advance         = tune_page1.advance;
        o.out_flags       = uint8_t((tune_page1.outputs_enable & 3) | (outputs::status.rev_limited ? 4 : 0)
                                  | (crank::status.simulating ? 8 : 0));
        o.pw_us           = tune_page1.inj_pw_us;
        o.sparks          = outputs::status.sparks;
        o.injections      = outputs::status.injections;
        o.spark_skipped   = outputs::status.spark_skipped;
        o.overlaps        = outputs::status.overlaps;
        o.spark_pulses    = outputs::status.spark_pulses;
        o.inj_pulses      = outputs::status.inj_pulses;
        o.sd_state        = sdcard::status.state;
        o.sd_error        = uint8_t(sdcard::status.last_error);
        o.sd_size_gb_x10  = sdcard::status.size_gb_x10;
        o.sd_free_gb_x10  = sdcard::status.free_gb_x10;
        o.sd_write_errors = sdcard::status.write_errors;
        o.power_state     = power::status.state;
        o.power_flags     = power::status.flags;
        o.sleep_in_ds     = power::status.sleep_in_ds;
    }

    bool handle_sig(Socket_t s);

    bool handle(Socket_t s, uint16_t len)
    {
        auto u16 = [](const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); };
        const uint8_t cmd = rx[0];

        switch (cmd)
        {
        case 'Q': case 'S':                                   // signature / version string
        {
            tx[0] = RC_OK;
            memcpy(&tx[1], SIGNATURE, sizeof(SIGNATURE) - 1);
            return send_frame(s, tx, sizeof(SIGNATURE));
        }
        case 'F':                                             // protocol version
        {
            const uint8_t v[] = { RC_OK, '0', '0', '2' };
            return send_frame(s, v, sizeof(v));
        }
        case 'I':                                             // CAN ID (0 = this ECU)
        {
            const uint8_t v[] = { RC_OK, 0 };
            return send_frame(s, v, sizeof(v));
        }
        case 'C':                                             // comms test
        {
            const uint8_t v[] = { RC_OK, 255 };
            return send_frame(s, v, sizeof(v));
        }
        case 'f':                                             // capabilities
        {
            const uint8_t v[] = { RC_OK, 2, uint8_t(BLOCKING >> 8), uint8_t(BLOCKING),
                                  uint8_t(TABLE_BLOCKING >> 8), uint8_t(TABLE_BLOCKING) };
            return send_frame(s, v, sizeof(v));
        }
        case 'r':                                             // live data
        {
            if (len < 7) return send_rc(s, RC_RANGE_ERR);
            uint16_t off = u16(&rx[3]), n = u16(&rx[5]);
            if (rx[2] == 0x0F) return handle_sig(s);
            if (rx[2] != 0x30 || off + n > sizeof(OutputChannels) || n > BLOCKING)
                return send_rc(s, RC_RANGE_ERR);
            OutputChannels o; fill_output_channels(o);
            tx[0] = RC_OK;
            memcpy(&tx[1], reinterpret_cast<uint8_t*>(&o) + off, n);
            return send_frame(s, tx, n + 1);
        }
        case 'p':                                             // read page
        {
            if (len < 7) return send_rc(s, RC_RANGE_ERR);
            uint16_t size; uint8_t* pg = page_ptr(rx[2], size);
            uint16_t off = u16(&rx[3]), n = u16(&rx[5]);
            if (!pg || off + n > size || n > BLOCKING) return send_rc(s, RC_RANGE_ERR);
            tx[0] = RC_OK;
            memcpy(&tx[1], pg + off, n);
            return send_frame(s, tx, n + 1);
        }
        case 'M':                                             // write to page (RAM)
        {
            if (len < 7) return send_rc(s, RC_RANGE_ERR);
            uint16_t size; uint8_t* pg = page_ptr(rx[2], size);
            uint16_t off = u16(&rx[3]), n = u16(&rx[5]);
            if (!pg || off + n > size || len < 7 + n) return send_rc(s, RC_RANGE_ERR);
            memcpy(pg + off, &rx[7], n);
            return send_rc(s, RC_OK);
        }
        case 'd':                                             // page CRC (big-endian)
        {
            uint16_t size; uint8_t* pg = page_ptr(rx[2], size);
            if (!pg) return send_rc(s, RC_RANGE_ERR);
            uint32_t c = crc32(pg, size);
            const uint8_t v[] = { RC_OK, uint8_t(c >> 24), uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c) };
            return send_frame(s, v, sizeof(v));
        }
        case 'b': case 'B':                                   // burn: TODO persist to flash
            return send_rc(s, RC_BURN_OK);
        default:
            return send_rc(s, RC_UNKNOWN);
        }
    }

    bool handle_sig(Socket_t s)
    {
        tx[0] = RC_OK;
        memcpy(&tx[1], SIGNATURE, sizeof(SIGNATURE) - 1);
        return send_frame(s, tx, sizeof(SIGNATURE));
    }

    void serve(Socket_t s)
    {
        for (;;)
        {
            uint8_t hdr[2];
            if (!recv_all(s, hdr, 1)) return;

            if (hdr[0] == 'F' || hdr[0] == 'Q' || hdr[0] == 'S')   // raw legacy probe
            {
                if (hdr[0] == 'F') { if (!send_all(s, (const uint8_t*)"002", 3)) return; }
                else { if (!send_all(s, (const uint8_t*)SIGNATURE, sizeof(SIGNATURE) - 1)) return; }
                continue;
            }

            if (!recv_all(s, &hdr[1], 1)) return;
            uint16_t len = uint16_t((hdr[0] << 8) | hdr[1]);
            if (len == 0 || len > sizeof(rx)) { send_rc(s, RC_RANGE_ERR); return; }

            uint8_t crcb[4];
            if (!recv_all(s, rx, len) || !recv_all(s, crcb, 4)) { send_rc(s, RC_TIMEOUT); return; }
            uint32_t got = (uint32_t(crcb[0]) << 24) | (crcb[1] << 16) | (crcb[2] << 8) | crcb[3];
            if (got != crc32(rx, len)) { if (!send_rc(s, RC_CRC_ERR)) return; continue; }

            if (!handle(s, len)) return;
        }
    }
}

// Defaults until burn-to-flash exists. Order follows TunePage1.
TunePage1 tune_page1 =
{
    60, 2,          // trigger wheel: 60-2
    5120,           // battery divider 41.2k/10k
    1500,           // 5 V sensor dividers 10k/20k
    1000,           // thermistor pull-up
    2050, 3500,     // Saab/Bosch NTC: R25, beta (fits T5 manual's IAT table)
    200, 4000,      // TPS closed / wide open, mV at sensor
    113, 2613,      // MAP 11.3 kPa @ 0 V .. 261.3 kPa @ 5 V (stock 2.5 bar)
    100, 200,       // wideband 10.0 AFR @ 0 V .. 20.0 AFR @ 5 V
    117,            // trigger angle: gap at ~117 deg BTDC cyl 1 (T5 manual); confirm with a timing light
    10,             // fixed advance, deg BTDC
    600,            // DI trigger pulse, us
    3000,           // injector pulse width, us
    0,              // injection start, deg BTDC of each pair
    6000,           // rev limit
    0,              // outputs disabled until enabled from TunerStudio
    0,              // crank simulator off
    0,              // bench ignition override off
    {}
};

static FreeRTOS::task<
    1024,
    configTIMER_TASK_PRIORITY
> _ts_task(
    [](void*)
    {
        outputs::init();
        crank::init();

        Socket_t listener = FreeRTOS_socket(FREERTOS_AF_INET4, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        configASSERT(listener != FREERTOS_INVALID_SOCKET);

        struct freertos_sockaddr addr = {};
        addr.sin_port = FreeRTOS_htons(TS_PORT);
        configASSERT(!FreeRTOS_bind(listener, &addr, sizeof(addr)));
        configASSERT(!FreeRTOS_listen(listener, 1));

        for (;;)
        {
            struct freertos_sockaddr client; socklen_t clen = sizeof(client);
            Socket_t s = FreeRTOS_accept(listener, &client, &clen);
            if (s == FREERTOS_INVALID_SOCKET || s == nullptr) continue;

            TickType_t to = pdMS_TO_TICKS(30000);         // drop clients idle > 30 s
            FreeRTOS_setsockopt(s, 0, FREERTOS_SO_RCVTIMEO, &to, sizeof(to));
            FreeRTOS_setsockopt(s, 0, FREERTOS_SO_SNDTIMEO, &to, sizeof(to));

            power::client_connected = true;
            serve(s);
            power::client_connected = false;
            FreeRTOS_shutdown(s, FREERTOS_SHUT_RDWR);
            FreeRTOS_closesocket(s);
        }
    },
    "TS_Link"
);