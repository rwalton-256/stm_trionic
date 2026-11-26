// Lockstep between the simulated clock and the FreeRTOS POSIX scheduler.
//
// Each simulated millisecond: deliver one tick, then wait until the idle task runs again, which
// means every firmware task has finished what that tick woke it up for. Peripherals (timers,
// crank edges) are only touched between ticks, while all tasks are blocked, so runs are
// deterministic and the firmware sees exactly one tick per simulated millisecond.
#pragma once
#include <cstdint>

namespace sim::rtos
{
    void     start();          // start the scheduler on its own thread, wait for first idle
    bool     started();
    void     tick();           // one tick, returns when the firmware is idle again
    uint32_t ticks();
}
