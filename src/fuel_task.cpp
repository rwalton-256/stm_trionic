// fuel_task.cpp - runs the fuel calculation every 10 ms (fuel.cpp itself has no RTOS code,
// so the sim can test it directly).
#include <FreeRTOS.hpp>
#include "fuel.hpp"

static FreeRTOS::task<
    512,
    tskIDLE_PRIORITY + 2
> _fuel_task(
    [](void*)
    {
        // Defaults are set at startup (fuel.cpp); the SD task may replace them with saved settings.
        TickType_t last = xTaskGetTickCount();
        for (;;)
        {
            fuel::update(0.01f);
            vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
        }
    },
    "Fuel"
);
