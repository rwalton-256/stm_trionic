#pragma once
#include <map>
#include <string>
#include <vector>
#include "settings.hpp"

namespace sim::board
{
    struct MemCard : settings::Io
    {
        std::map<std::string, std::vector<uint8_t>> files;
        bool   present = true;
        size_t tear_next_write = 0;     // >0: next write stops after this many bytes
        int    writes = 0;
        int  read(const char* name, uint8_t* buf, size_t max) override;
        bool write(const char* name, const uint8_t* buf, size_t len) override;
    };
    extern MemCard card;
    extern bool    stopped, reset;
}
