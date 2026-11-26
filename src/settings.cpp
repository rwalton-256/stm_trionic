// settings.cpp - see settings.hpp
#include <cstring>
#include "settings.hpp"
#include "tune.hpp"
#include "fuel.hpp"

extern "C" __attribute__((weak)) void settings_lock()   {}
extern "C" __attribute__((weak)) void settings_unlock() {}

namespace settings
{
    Status status{};
    const char* const FILE_NAMES[2] = { "ECU/tune_a.bin", "ECU/tune_b.bin" };

    namespace
    {
        struct __attribute__((packed)) Header
        {
            uint32_t magic;
            uint16_t version;
            uint16_t tune_size;
            uint16_t tables_size;
            uint16_t reserved;
            uint32_t seq;
            uint32_t crc;            // CRC32 of the payload (tune + tables)
        };
        constexpr size_t PAYLOAD = sizeof(TunePage1) + sizeof(fuel::Tables);
        constexpr size_t RECORD  = sizeof(Header) + PAYLOAD;

        volatile bool save_req = false;
        uint32_t last_saved_crc = 0;
        bool     have_saved     = false;
        int      newest_slot    = -1;     // slot holding the record in use (-1 = none)

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

        // Bench-only switches never come back after a power cycle.
        void sanitize(TunePage1& t)
        {
            t.sim_rpm   = 0;
            t.bench_ign = 0;
        }

        alignas(4) uint8_t scratch[2][RECORD];
    }

    size_t record_size() { return RECORD; }

    size_t build_record(uint8_t* buf, uint32_t seq)
    {
        uint8_t* payload = buf + sizeof(Header);
        settings_lock();
        std::memcpy(payload, &tune_page1, sizeof(TunePage1));
        std::memcpy(payload + sizeof(TunePage1), &fuel::tables, sizeof(fuel::Tables));
        settings_unlock();
        sanitize(*reinterpret_cast<TunePage1*>(payload));

        Header h{ MAGIC, VERSION, uint16_t(sizeof(TunePage1)), uint16_t(sizeof(fuel::Tables)), 0, seq,
                  crc32(payload, PAYLOAD) };
        std::memcpy(buf, &h, sizeof(h));
        return RECORD;
    }

    bool parse_record(const uint8_t* buf, size_t len, uint32_t& seq)
    {
        if (len != RECORD) return false;
        Header h; std::memcpy(&h, buf, sizeof(h));
        if (h.magic != MAGIC || h.version != VERSION) return false;
        if (h.tune_size != sizeof(TunePage1) || h.tables_size != sizeof(fuel::Tables)) return false;
        if (crc32(buf + sizeof(Header), PAYLOAD) != h.crc) return false;
        seq = h.seq;
        return true;
    }

    void apply_record(const uint8_t* buf)
    {
        const uint8_t* payload = buf + sizeof(Header);
        TunePage1 t; std::memcpy(&t, payload, sizeof(t));
        sanitize(t);
        settings_lock();
        tune_page1 = t;
        std::memcpy(&fuel::tables, payload + sizeof(TunePage1), sizeof(fuel::Tables));
        settings_unlock();
    }

    bool load(Io& io, bool apply)
    {
        uint32_t seq[2]; bool ok[2];
        for (int i = 0; i < 2; i++)
        {
            int n = io.read(FILE_NAMES[i], scratch[i], RECORD);
            ok[i] = n == int(RECORD) && parse_record(scratch[i], size_t(n), seq[i]);
        }
        int pick = ok[0] && ok[1] ? (int32_t(seq[1] - seq[0]) > 0 ? 1 : 0)   // wrap-safe newest
                 : ok[0] ? 0 : ok[1] ? 1 : -1;
        if (pick < 0) { newest_slot = -1; status.source = DEFAULTS; return false; }

        newest_slot = pick;                    // the next save goes to the other file
        status.seq  = seq[pick];
        if (!apply) return true;               // engine already running: keep the live settings

        apply_record(scratch[pick]);
        status.source  = uint8_t(pick == 0 ? FILE_A : FILE_B);
        last_saved_crc = crc32(scratch[pick] + sizeof(Header), PAYLOAD);
        have_saved     = true;
        status.dirty   = false;
        return true;
    }

    bool save(Io& io)
    {
        uint32_t next = newest_slot < 0 ? 1 : status.seq + 1;
        int slot = newest_slot < 0 ? 0 : 1 - newest_slot;                    // overwrite the older one
        build_record(scratch[slot], next);
        if (!io.write(FILE_NAMES[slot], scratch[slot], RECORD)) { status.save_errors++; return false; }

        newest_slot    = slot;
        status.seq     = next;
        status.source  = uint8_t(slot == 0 ? FILE_A : FILE_B);
        status.saves++;
        std::memcpy(&last_saved_crc, scratch[slot] + offsetof(Header, crc), 4);
        have_saved     = true;
        status.dirty   = false;
        return true;
    }

    void service(Io& io, uint32_t now_ms, bool ign_on, bool engine_turning)
    {
        static bool     started = false, was_on = false;
        static uint32_t last_save = 0;
        if (!started)
        {
            started = true;
            bool ok = load(io, !engine_turning);
            if (!ok || engine_turning)
            {
                // Nothing applied: the live settings are the baseline, unsaved until they change.
                uint8_t* buf = scratch[0];
                build_record(buf, 0);
                std::memcpy(&last_saved_crc, buf + offsetof(Header, crc), 4);
                have_saved = true;
            }
            was_on = ign_on; last_save = now_ms;
            return;
        }
        refresh_dirty();
        bool key_off  = was_on && !ign_on;
        was_on        = ign_on;
        bool burn     = take_save_request();
        bool periodic = status.dirty && now_ms - last_save >= SAVE_PERIOD_MS;
        if (burn || (key_off && status.dirty) || periodic)
        {
            save(io);
            last_save = now_ms;
        }
    }

    void request_save()      { save_req = true; }
    bool take_save_request() { bool r = save_req; save_req = false; return r; }

    void refresh_dirty()
    {
        uint8_t* buf = scratch[0];
        build_record(buf, 0);
        uint32_t c; std::memcpy(&c, buf + offsetof(Header, crc), 4);
        status.dirty = !have_saved || c != last_saved_crc;
    }
}
