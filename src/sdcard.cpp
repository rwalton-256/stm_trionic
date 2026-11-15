// sdcard.cpp - microSD on the Olimex STM32-E407 slot (SDIO, 4-bit) with FatFs on top
//
//   PC8..PC11 = D0..D3, PC12 = CLK, PD2 = CMD (all AF12). The slot has no card-detect line,
//   so a card is "present" when it initialises. Card must be FAT32 (format it on a PC).
//
// Transfers use HAL polling, one 512-byte block at a time inside a FreeRTOS critical section:
// that keeps other tasks (and the Ethernet interrupt) from stalling the SDIO FIFO mid-block,
// while the engine interrupts (priority 1, above the RTOS ceiling) still run on time.
// SDIO clock 48 MHz / (4 + 2) = 8 MHz, ~4 MB/s in 4-bit mode: ~130 us per block.
//
// HAL must be included before FreeRTOS.hpp (SystemCoreClock linkage clash in FreeRTOSConfig.h).
#include <stm32f4xx_hal.h>
#include <FreeRTOS.hpp>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ff.h"
#include "diskio.h"
#include "sdcard.hpp"

namespace sdcard
{
    Status status{};
}

namespace
{
    SD_HandleTypeDef hsd;
    volatile bool    card_ok = false;
    FATFS            fs;
    FIL              file;

    void pins_init()
    {
        __HAL_RCC_SDIO_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitTypeDef g{};
        g.Mode      = GPIO_MODE_AF_PP;
        g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF12_SDIO;
        g.Pull      = GPIO_PULLUP;
        g.Pin       = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;   // D0..D3
        HAL_GPIO_Init(GPIOC, &g);
        g.Pin       = GPIO_PIN_2;                                           // CMD
        HAL_GPIO_Init(GPIOD, &g);
        g.Pull      = GPIO_NOPULL;
        g.Pin       = GPIO_PIN_12;                                          // CLK
        HAL_GPIO_Init(GPIOC, &g);
    }

    bool card_init()
    {
        HAL_SD_DeInit(&hsd);
        hsd.Instance                 = SDIO;
        hsd.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
        hsd.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
        hsd.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
        hsd.Init.BusWide             = SDIO_BUS_WIDE_1B;                    // must start 1-bit
        hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
        hsd.Init.ClockDiv            = 4;                                   // 8 MHz after init
        if (HAL_SD_Init(&hsd) != HAL_OK) return false;
        if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK) return false;
        return true;
    }

    // Wait for the card to finish an operation (programming after a write can take a while).
    bool wait_ready(uint32_t timeout_ms)
    {
        TickType_t start = xTaskGetTickCount();
        while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER)
        {
            if (xTaskGetTickCount() - start > pdMS_TO_TICKS(timeout_ms)) return false;
            vTaskDelay(1);
        }
        return true;
    }

    alignas(4) uint8_t bounce[512];   // HAL polling is happiest with word-aligned buffers
}

// ---- FatFs disk I/O layer (one physical drive: 0) ----------------------------------------
extern "C"
{
DSTATUS disk_status(BYTE pdrv)
{
    return (pdrv == 0 && card_ok) ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    card_ok = card_init();
    return card_ok ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || !card_ok) return RES_NOTRDY;
    for (UINT i = 0; i < count; i++)
    {
        if (!wait_ready(500)) return RES_ERROR;
        taskENTER_CRITICAL();
        HAL_StatusTypeDef r = HAL_SD_ReadBlocks(&hsd, bounce, uint32_t(sector + i), 1, 100);
        taskEXIT_CRITICAL();
        if (r != HAL_OK) return RES_ERROR;
        memcpy(buff + i * 512, bounce, 512);
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || !card_ok) return RES_NOTRDY;
    for (UINT i = 0; i < count; i++)
    {
        memcpy(bounce, buff + i * 512, 512);
        if (!wait_ready(500)) return RES_ERROR;
        taskENTER_CRITICAL();
        HAL_StatusTypeDef r = HAL_SD_WriteBlocks(&hsd, bounce, uint32_t(sector + i), 1, 100);
        taskEXIT_CRITICAL();
        if (r != HAL_OK) { sdcard::status.write_errors++; return RES_ERROR; }
    }
    return wait_ready(1000) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != 0 || !card_ok) return RES_NOTRDY;
    switch (cmd)
    {
    case CTRL_SYNC:        return wait_ready(1000) ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT: *static_cast<LBA_t*>(buff) = hsd.SdCard.LogBlockNbr; return RES_OK;
    case GET_SECTOR_SIZE:  *static_cast<WORD*>(buff)  = 512;                    return RES_OK;
    case GET_BLOCK_SIZE:   *static_cast<DWORD*>(buff) = 1;                      return RES_OK;
    default:               return RES_PARERR;
    }
}

// No real-time clock is set yet: stamp files 2026-01-01 00:00.
DWORD get_fattime(void)
{
    return (DWORD(2026 - 1980) << 25) | (1u << 21) | (1u << 16);
}
}

// ---- the SD task: mount, self-test, keep status up to date -------------------------------
namespace
{
    FRESULT self_test()
    {
        FRESULT r = f_mkdir("ECU");
        if (r != FR_OK && r != FR_EXIST) return r;

        // Boot counter: proves read and write both work, and is handy for naming logs later.
        uint32_t boots = 0;
        UINT n = 0;
        if (f_open(&file, "ECU/boots.txt", FA_READ) == FR_OK)
        {
            char line[16] = {};
            f_read(&file, line, sizeof(line) - 1, &n);
            f_close(&file);
            boots = uint32_t(strtoul(line, nullptr, 10));
        }
        char line[16];
        int len = snprintf(line, sizeof(line), "%lu\n", (unsigned long)(boots + 1));
        if ((r = f_open(&file, "ECU/boots.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) return r;
        r = f_write(&file, line, UINT(len), &n);
        FRESULT c = f_close(&file);
        if (r != FR_OK) return r;
        if (n != UINT(len)) return FR_DISK_ERR;
        return c;
    }

    void update_space()
    {
        DWORD free_clusters = 0;
        FATFS* pfs = nullptr;
        if (f_getfree("", &free_clusters, &pfs) == FR_OK)
        {
            uint64_t total = uint64_t(pfs->n_fatent - 2) * pfs->csize * 512;
            uint64_t freeb = uint64_t(free_clusters)     * pfs->csize * 512;
            sdcard::status.size_gb_x10 = uint16_t(total * 10 / 1000000000ull);
            sdcard::status.free_gb_x10 = uint16_t(freeb * 10 / 1000000000ull);
        }
    }
}

static FreeRTOS::task<
    1024,
    tskIDLE_PRIORITY + 1
> _sd_task(
    [](void*)
    {
        pins_init();
        for (;;)
        {
            if (sdcard::status.state != sdcard::READY)
            {
                // (Re)try every 2 s, so a card can be inserted after power-up.
                f_mount(nullptr, "", 0);
                FRESULT r = f_mount(&fs, "", 1);                  // 1 = mount now
                sdcard::status.last_error = uint16_t(r);
                if (r == FR_OK)
                {
                    r = self_test();
                    sdcard::status.last_error = uint16_t(r);
                    sdcard::status.state = (r == FR_OK) ? sdcard::READY : sdcard::IO_ERROR;
                    if (r == FR_OK) update_space();
                }
                else
                {
                    sdcard::status.state = (r == FR_NOT_READY) ? sdcard::NO_CARD : sdcard::MOUNT_FAILED;
                }
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            else
            {
                // Card present and mounted: check it's still there every few seconds.
                if (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER)
                {
                    card_ok = false;
                    sdcard::status.state = sdcard::NO_CARD;
                }
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
    },
    "SDCard"
);