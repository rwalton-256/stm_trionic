#include "lockstep.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unistd.h>
extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

namespace
{
    // Only shared by the Python thread and the tick thread, never taken by a task (a tick signal
    // can suspend a task anywhere). Never destroyed: at process exit the tick thread is still
    // waiting, and glibc's condition-variable destructor would wait for it forever.
    std::mutex&              m  = *new std::mutex;
    std::condition_variable& cv = *new std::condition_variable;
    bool                    tick_pending = false;
    std::atomic<int64_t>    idle_at{ -1 };  // tick count last seen by the idle task
    bool                    running = false;
    uint32_t                n_ticks = 0;

    void wait_idle(int64_t tick, const char* what)
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (idle_at.load(std::memory_order_acquire) < tick)
        {
            if (std::chrono::steady_clock::now() > deadline)
                throw std::runtime_error(std::string("firmware never went idle ") + what +
                                         " (a task is busy-looping without blocking?)");
            std::this_thread::yield();
        }
    }
}

// Called by the port's tick thread: block until the simulated clock wants a tick.
extern "C" int sim_lockstep_wait_tick(void)
{
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, [] { return tick_pending; });
    tick_pending = false;
    return 1;
}

// Idle task: lock-free only, a tick signal can suspend this thread at any instruction.
extern "C" void vApplicationIdleHook(void)
{
    idle_at.store(int64_t(xTaskGetTickCount()), std::memory_order_release);
    usleep(20);
}

namespace sim::rtos
{
    void start()
    {
        if (running) return;
        running = true;
        std::thread([] { vTaskStartScheduler(); }).detach();
        wait_idle(0, "after start-up");
    }
    bool     started() { return running; }
    uint32_t ticks()   { return n_ticks; }

    void tick()
    {
        int64_t target = int64_t(n_ticks) + 1;
        { std::lock_guard<std::mutex> l(m); tick_pending = true; }
        cv.notify_one();
        wait_idle(target, "after a tick");
        n_ticks = uint32_t(target);
    }
}
