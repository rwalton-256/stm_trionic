// Persistent settings (tune page + fuel tables) on the SD card, two alternating files.
//
//   ECU/tune_a.bin, ECU/tune_b.bin: [header][TunePage1][fuel::Tables]
//   header = magic, layout version, sizes, sequence number, CRC32 of the payload
//
// Save always overwrites the OLDER file with seq+1, so a power cut mid-write can only damage
// that one; load picks the newest file whose CRC checks out, else keeps the defaults.
// No FatFs or RTOS code here (src/sdcard.cpp provides the file I/O), so the sim can test it.
#pragma once
#include <cstddef>
#include <cstdint>

namespace settings
{
    struct Io
    {
        virtual int  read(const char* name, uint8_t* buf, size_t max) = 0;     // bytes read, <0 = missing
        virtual bool write(const char* name, const uint8_t* buf, size_t len) = 0;
        virtual ~Io() = default;
    };

    enum Source : uint8_t { DEFAULTS = 0, FILE_A = 1, FILE_B = 2 };

    struct Status
    {
        volatile uint32_t seq;           // sequence number of the record in use
        volatile uint8_t  source;        // where the live settings came from
        volatile uint16_t saves;
        volatile uint16_t save_errors;
        volatile bool     dirty;         // live settings differ from the last save
    };
    extern Status status;

    extern const char* const FILE_NAMES[2];
    constexpr uint32_t MAGIC   = 0x4E525453;   // "STRN"
    constexpr uint16_t VERSION = 1;            // bump when TunePage1 / fuel::Tables layout changes

    // Call about once a second from the task that owns the card. First call: load the saved
    // settings (or, if the engine is already turning, only note which file is newest and keep
    // the live settings). Then save on: TunerStudio burn, key-off with changes, or every
    // SAVE_PERIOD_MS while settings keep changing (VE learning).
    constexpr uint32_t SAVE_PERIOD_MS = 60000;
    void service(Io& io, uint32_t now_ms, bool ign_on, bool engine_turning);

    bool load(Io& io, bool apply = true);   // newest valid record; false = no valid file
    bool save(Io& io);               // write the older file
    void request_save();             // TunerStudio "burn" (any context)
    bool take_save_request();
    void refresh_dirty();            // compare live settings with the last save

    // Building blocks (exposed for tests)
    size_t record_size();
    size_t build_record(uint8_t* buf, uint32_t seq);                     // snapshot of live settings
    bool   parse_record(const uint8_t* buf, size_t len, uint32_t& seq);  // validate only
    void   apply_record(const uint8_t* buf);
}

// Provided by the firmware (critical section around snapshot/apply); weak no-ops otherwise.
extern "C" void settings_lock();
extern "C" void settings_unlock();
