// ota.cpp - firmware update over Ethernet (TCP port 29002), no bootloader needed
//
// Flash layout (STM32F407, 1 MB, single bank):
//   sectors 0..7   0x08000000..0x0807FFFF  512 KB  running application (linker limit: 512K)
//   sectors 8..11  0x08080000..0x080FFFFF  512 KB  staging area for the new image
//
// Sequence (see tools/ota_upload.py):
//   1. client -> "OTA1", size (u32 LE), crc32 (u32 LE)
//   2. ECU refuses ('B') unless the engine is stopped; otherwise turns all outputs off,
//      erases staging and replies 'E'. Erasing stalls the CPU for a few seconds: expected.
//   3. client streams the image; ECU programs it into staging, then checks the CRC.
//   4. on a good CRC the ECU replies 'V', then a routine running from RAM (interrupts off)
//      copies staging over the application and resets. Takes ~5 s.
//   If power is lost during step 4 the board needs JTAG to recover. A bad upload or CRC
//   never touches the running application.
//
// HAL must be included before FreeRTOS.hpp (SystemCoreClock linkage clash in FreeRTOSConfig.h).
#include <stm32f4xx_hal.h>
#include <FreeRTOS.hpp>
#include <cstring>
#include "crank.hpp"
#include "outputs.hpp"
#include "tune.hpp"
#include "power.hpp"

namespace
{
    constexpr uint16_t OTA_PORT     = 29002;
    constexpr uint32_t APP_BASE     = 0x08000000;
    constexpr uint32_t STAGING_BASE = 0x08080000;
    constexpr uint32_t MAX_IMAGE    = 512 * 1024;
    constexpr uint32_t FIRST_STAGING_SECTOR = 8;

    uint8_t buf[1024];

    // Start address of flash sector n (F407: 4 x 16K, 1 x 64K, 7 x 128K)
    constexpr uint32_t sector_start(uint32_t n)
    {
        return n < 4 ? 0x08000000 + n * 0x4000
             : n == 4 ? 0x08010000
             : 0x08020000 + (n - 5) * 0x20000;
    }

    uint32_t crc32_update(uint32_t c, const uint8_t* p, size_t n)
    {
        while (n--)
        {
            c ^= *p++;
            for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
        return c;
    }

    // ---- flash, running from flash (CPU stalls during erase/program; that's fine) ----
    void flash_unlock()
    {
        if (FLASH->CR & FLASH_CR_LOCK)
        {
            FLASH->KEYR = 0x45670123;
            FLASH->KEYR = 0xCDEF89AB;
        }
    }

    constexpr uint32_t SR_ERRORS = FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR;

    bool flash_wait()
    {
        while (FLASH->SR & FLASH_SR_BSY) {}
        bool ok = !(FLASH->SR & SR_ERRORS);
        FLASH->SR = SR_ERRORS | FLASH_SR_EOP;
        return ok;
    }

    bool erase_sector(uint32_t n)
    {
        flash_wait();
        FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_SER | (n << FLASH_CR_SNB_Pos);
        FLASH->CR |= FLASH_CR_STRT;
        bool ok = flash_wait();
        FLASH->CR = 0;
        return ok;
    }

    bool program_word(uint32_t addr, uint32_t word)
    {
        FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_PG;
        *reinterpret_cast<volatile uint32_t*>(addr) = word;
        bool ok = flash_wait();
        FLASH->CR = 0;
        return ok;
    }

    void flush_flash_caches()
    {
        FLASH->ACR &= ~(FLASH_ACR_DCEN | FLASH_ACR_ICEN);
        FLASH->ACR |=  (FLASH_ACR_DCRST | FLASH_ACR_ICRST);
        FLASH->ACR &= ~(FLASH_ACR_DCRST | FLASH_ACR_ICRST);
        FLASH->ACR |=  (FLASH_ACR_DCEN | FLASH_ACR_ICEN);
    }

    // ---- the installer: runs entirely from RAM with interrupts off ----
    // Only register accesses and volatile loops in here: no calls into flash, no memcpy.
    __attribute__((section(".RamFunc"), noinline, long_call, used))
    void ram_install(uint32_t size)
    {
        FLASH->SR = 0xF3;
        for (uint32_t n = 0; n < FIRST_STAGING_SECTOR; n++)
        {
            uint32_t start = n < 4 ? 0x08000000 + n * 0x4000 : n == 4 ? 0x08010000 : 0x08020000 + (n - 5) * 0x20000;
            if (start >= APP_BASE + size) break;
            while (FLASH->SR & FLASH_SR_BSY) {}
            FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_SER | (n << FLASH_CR_SNB_Pos);
            FLASH->CR |= FLASH_CR_STRT;
            while (FLASH->SR & FLASH_SR_BSY) {}
        }
        FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_PG;
        volatile uint32_t* src = reinterpret_cast<volatile uint32_t*>(STAGING_BASE);
        volatile uint32_t* dst = reinterpret_cast<volatile uint32_t*>(APP_BASE);
        uint32_t words = (size + 3) >> 2;
        for (uint32_t i = 0; i < words; i++)
        {
            dst[i] = src[i];
            while (FLASH->SR & FLASH_SR_BSY) {}
        }
        FLASH->CR = 0;
        __DSB();
        SCB->AIRCR = (0x5FAUL << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk;
        __DSB();
        for (;;) {}
    }

    // ---- socket helpers ----
    bool recv_all(Socket_t s, uint8_t* p, size_t n)
    {
        while (n)
        {
            BaseType_t r = FreeRTOS_recv(s, p, n, 0);
            if (r <= 0) return false;
            p += r; n -= size_t(r);
        }
        return true;
    }

    void reply(Socket_t s, char c) { FreeRTOS_send(s, &c, 1, 0); }

    void session(Socket_t s)
    {
        uint8_t hdr[12];
        if (!recv_all(s, hdr, 4)) return;
        if (memcmp(hdr, "OTAQ", 4) == 0)                                        // status query
        {
            uint32_t uptime = xTaskGetTickCount() / configTICK_RATE_HZ;
            FreeRTOS_send(s, &uptime, sizeof(uptime), 0);
            return;
        }
        if (memcmp(hdr, "OTA1", 4) != 0 || !recv_all(s, &hdr[4], 8)) { reply(s, '?'); return; }
        uint32_t size, crc;
        memcpy(&size, &hdr[4], 4);
        memcpy(&crc,  &hdr[8], 4);

        if (size < 1024 || size > MAX_IMAGE) { reply(s, 'S'); return; }        // bad size
        bool real_engine_turning = !crank::status.simulating && (crank::status.synced || crank::status.rpm != 0);
        if (real_engine_turning) { reply(s, 'B'); return; }                     // engine turning

        power::update_in_progress = true;                                       // don't sleep mid-update
        struct Clear { ~Clear() { power::update_in_progress = false; } } clear_on_exit;

        // Make sure nothing can fire while the CPU stalls on flash operations.
        tune_page1.outputs_enable = 0;
        tune_page1.sim_rpm        = 0;
        outputs::stop();

        flash_unlock();
        for (uint32_t n = FIRST_STAGING_SECTOR; n < 12; n++)
        {
            if (sector_start(n) >= STAGING_BASE + size) break;
            if (!erase_sector(n)) { reply(s, 'X'); return; }                    // erase failed
        }
        reply(s, 'E');

        uint32_t addr = STAGING_BASE, left = size;
        while (left)
        {
            uint32_t n = left < sizeof(buf) ? left : sizeof(buf);
            if (!recv_all(s, buf, n)) return;                                   // client gone
            if (n & 3) memset(&buf[n], 0xFF, 4 - (n & 3));                     // pad last word
            for (uint32_t i = 0; i < n; i += 4)
            {
                uint32_t w; memcpy(&w, &buf[i], 4);
                if (!program_word(addr + i, w)) { reply(s, 'W'); return; }      // write failed
            }
            addr += n; left -= n;
        }

        flush_flash_caches();
        uint32_t got = ~crc32_update(0xFFFFFFFFu, reinterpret_cast<const uint8_t*>(STAGING_BASE), size);
        if (got != crc) { reply(s, 'C'); return; }                             // CRC mismatch

        reply(s, 'V');
        vTaskDelay(pdMS_TO_TICKS(200));                                         // let the reply go out
        FreeRTOS_shutdown(s, FREERTOS_SHUT_RDWR);
        vTaskDelay(pdMS_TO_TICKS(100));

        __disable_irq();
        ram_install(size);                                                      // does not return
    }
}

static FreeRTOS::task<
    512,
    tskIDLE_PRIORITY + 2
> _ota_task(
    [](void*)
    {
        Socket_t listener = FreeRTOS_socket(FREERTOS_AF_INET4, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        configASSERT(listener != FREERTOS_INVALID_SOCKET);
        struct freertos_sockaddr addr = {};
        addr.sin_port = FreeRTOS_htons(OTA_PORT);
        configASSERT(!FreeRTOS_bind(listener, &addr, sizeof(addr)));
        configASSERT(!FreeRTOS_listen(listener, 2));   // room for the next client while the last one closes

        for (;;)
        {
            struct freertos_sockaddr client; socklen_t clen = sizeof(client);
            Socket_t s = FreeRTOS_accept(listener, &client, &clen);
            if (s == FREERTOS_INVALID_SOCKET || s == nullptr) continue;
            TickType_t to = pdMS_TO_TICKS(30000);
            FreeRTOS_setsockopt(s, 0, FREERTOS_SO_RCVTIMEO, &to, sizeof(to));
            FreeRTOS_setsockopt(s, 0, FREERTOS_SO_SNDTIMEO, &to, sizeof(to));
            session(s);
            FreeRTOS_shutdown(s, FREERTOS_SHUT_RDWR);
            FreeRTOS_closesocket(s);
        }
    },
    "OTA"
);
