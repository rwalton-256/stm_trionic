#pragma once
#include <cstdint>

// microSD card on the Olimex SDIO slot, FAT32 via FatFs (src/sdcard.cpp).
namespace sdcard
{
    enum State : uint8_t { STARTING = 0, READY = 1, NO_CARD = 2, MOUNT_FAILED = 3, IO_ERROR = 4 };

    struct Status
    {
        volatile uint8_t  state;
        volatile uint16_t size_gb_x10;   // card capacity
        volatile uint16_t free_gb_x10;   // free space on the FAT volume
        volatile uint16_t last_error;    // last FatFs FRESULT (0 = OK)
        volatile uint16_t write_errors;
    };
    extern Status status;
}