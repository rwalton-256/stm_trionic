// The rest of the board for the simulated ECU: registers the power code touches, sleep/reset
// hooks, a virtual SD card running the real settings code, and stand-ins for modules not
// simulated yet (sensors: values come from Python).
#include "stm32f4xx_hal.h"
#include <FreeRTOS.hpp>
#include "board.hpp"
#include "settings.hpp"
#include "sensors.hpp"
#include "sdcard.hpp"
#include "power.hpp"
#include "crank.hpp"

ETH_TypeDef sim_eth{}; SYSCFG_TypeDef sim_syscfg{}; EXTI_TypeDef sim_exti{}; SysTick_Type sim_systick{};

namespace sensors { Readings readings{}; }
namespace sdcard  { Status   status{};   }

namespace sim::board
{
    bool    stopped = false;          // power.cpp entered STOP mode
    bool    reset   = false;          // ...and asked for a reset (wake)
    MemCard card;

    int MemCard::read(const char* n, uint8_t* buf, size_t max)
    {
        if (!present || !files.count(n)) return -1;
        auto& f = files[n];
        size_t k = f.size() < max ? f.size() : max;
        std::copy(f.begin(), f.begin() + long(k), buf);
        return int(k);
    }
    bool MemCard::write(const char* n, const uint8_t* buf, size_t len)
    {
        if (!present) return false;
        size_t k = tear_next_write ? tear_next_write : len;     // simulated power cut mid-write
        tear_next_write = 0;
        files[n].assign(buf, buf + (k < len ? k : len));
        writes++;
        return k >= len;
    }
}

void sim_enter_stop() { sim::board::stopped = true; }

// NVIC_SystemReset from power.cpp's sleep: the sim can't reboot the scheduler, so the power task
// just parks here (scheduler resumed so the other tasks and the lockstep keep running).
void sim_system_reset()
{
    sim::board::reset = true;
    xTaskResumeAll();
    for (;;) vTaskSuspend(nullptr);
}

// Socket shim waits hand control back to the scheduler (one poll per tick).
extern "C" void sim_net_yield() { vTaskDelay(1); }

extern "C" void vAssertCalled(const char* file, unsigned long line)
{
    std::fprintf(stderr, "ASSERT %s:%lu\n", file, line); std::abort();
}

// Settings snapshot/apply vs the fuel task updating VE (src/sdcard.cpp does the same on hardware).
extern "C" void settings_lock()   { taskENTER_CRITICAL(); }
extern "C" void settings_unlock() { taskEXIT_CRITICAL(); }

// Same job as the SD task in src/sdcard.cpp, with the card in memory.
static FreeRTOS::task<1024, tskIDLE_PRIORITY + 1> _sd_task(
    [](void*)
    {
        for (;;)
        {
            auto& c = sim::board::card;
            sdcard::status.state = c.present ? sdcard::READY : sdcard::NO_CARD;
            if (c.present)
                settings::service(c, xTaskGetTickCount() * portTICK_PERIOD_MS,
                                  power::status.state == power::IGN_ON, crank::status.rpm != 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }, "SDCard");
