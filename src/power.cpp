// power.cpp - ignition sense, main/fuel relays, run-on and STOP-mode sleep (see power.hpp)
//
// HAL must be included before FreeRTOS.hpp (SystemCoreClock linkage clash in FreeRTOSConfig.h).
#include <stm32f4xx_hal.h>
#include <FreeRTOS.hpp>
#include "power.hpp"
#include "crank.hpp"
#include "outputs.hpp"
#include "tune.hpp"

namespace power
{
    Status        status{};
    volatile bool client_connected   = false;
    volatile bool update_in_progress = false;

    bool outputs_allowed()
    {
        return status.state == IGN_ON && (status.flags & F_MAIN_RELAY);
    }
}

namespace
{
    using namespace power;

    constexpr uint32_t TICK_MS        = 10;
    constexpr uint32_t IGN_ON_MS      = 30;     // debounce: +15 must be high this long
    constexpr uint32_t IGN_OFF_MS     = 100;    // ...and low this long (noise filter; wiring is solid)
    constexpr uint32_t PRIME_MS       = 2000;   // fuel pump prime at key-on
    constexpr uint32_t RUNON_MS       = 5000;   // ignition off this long before sleeping
    constexpr uint32_t BOOT_GRACE_MS  = 30000;  // stay awake after power-up so TunerStudio can connect
    constexpr uint32_t CRANK_RPM_MAX  = 400;    // below this the engine is cranking, not running
    constexpr uint32_t CRANK_HOLD_MS  = 3000;   // longest a +15 dip is ignored while cranking

    void relay(uint16_t pin, bool on) { HAL_GPIO_WritePin(GPIOG, pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET); }

    void pins_init()
    {
        __HAL_RCC_GPIOG_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7 | GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);

        GPIO_InitTypeDef g{};
        g.Mode  = GPIO_MODE_OUTPUT_PP;
        g.Pull  = GPIO_NOPULL;
        g.Speed = GPIO_SPEED_FREQ_LOW;
        g.Pin   = GPIO_PIN_7 | GPIO_PIN_8;        // main relay, fuel pump relay
        HAL_GPIO_Init(GPIOG, &g);
        g.Pin   = GPIO_PIN_5 | GPIO_PIN_6;        // boost valve A/B: not used yet, held off
        HAL_GPIO_Init(GPIOE, &g);

        g.Mode  = GPIO_MODE_INPUT;                // +15 sense (divider provides the pull-down)
        g.Pin   = GPIO_PIN_12;
        HAL_GPIO_Init(GPIOG, &g);
    }

    bool ign_input() { return HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_12) == GPIO_PIN_SET; }

    // Stop the injector/spark timers (pins stay with the timers, driven low). Interrupts off
    // briefly so a crank interrupt can't re-arm a channel halfway through.
    void stop_outputs()
    {
        __disable_irq();
        outputs::stop();
        __enable_irq();
    }

    // For sleep: every MOSFET gate as a plain output, low. (They also have 1k pull-downs.)
    // Only used right before STOP mode; the restart on wake sets the pins up again.
    void all_outputs_low()
    {
        stop_outputs();
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7 | GPIO_PIN_8, GPIO_PIN_RESET);
        GPIO_InitTypeDef g{};
        g.Mode  = GPIO_MODE_OUTPUT_PP;
        g.Pull  = GPIO_NOPULL;
        g.Speed = GPIO_SPEED_FREQ_LOW;
        g.Pin   = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;               // injectors
        HAL_GPIO_Init(GPIOD, &g);
        g.Pin   = GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_5 | GPIO_PIN_6;  // spark, boost
        HAL_GPIO_Init(GPIOE, &g);
    }

    // LAN8710 power-down (BMCR bit 11) over MDIO, PHY address 0, keeping the MDC clock divider.
    void phy_power_down()
    {
        uint32_t cr = ETH->MACMIIAR & ETH_MACMIIAR_CR;
        ETH->MACMIIDR = 1u << 11;
        ETH->MACMIIAR = (0u << 11) | (0u << 6) | cr | ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;
        for (uint32_t i = 0; i < 1000000 && (ETH->MACMIIAR & ETH_MACMIIAR_MB); i++) {}
    }

    [[noreturn]] void sleep_now()
    {
        status.state = SLEEPING;
        all_outputs_low();
        vTaskDelay(pdMS_TO_TICKS(300));            // let the SD card / network finish up
        phy_power_down();

        vTaskSuspendAll();
        __disable_irq();

        // Wake on a rising edge of +15 (PG12 = EXTI line 12), as an event so no ISR is needed.
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI12) | SYSCFG_EXTICR4_EXTI12_PG;
        EXTI->IMR  &= ~(1u << 12);
        EXTI->EMR  |=  (1u << 12);
        EXTI->RTSR |=  (1u << 12);
        EXTI->FTSR &= ~(1u << 12);
        EXTI->PR    =  (1u << 12);

        if (!ign_input())                          // don't sleep if the key came back meanwhile
        {
            SysTick->CTRL = 0;
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
        }
        NVIC_SystemReset();                        // woken (or key back on): clean restart
    }
}

static FreeRTOS::task<
    512,
    tskIDLE_PRIORITY + 3
> _power_task(
    [](void*)
    {
        pins_init();

        uint32_t on_ms = 0, off_ms = 0, prime_ms = 0, off_for_ms = 0;
        bool     ign = false;

        for (uint32_t now_ms = 0; ; now_ms += TICK_MS)
        {
            bool raw     = ign_input();
            bool bench   = tune_page1.bench_ign != 0;
            bool turning = crank::status.synced || crank::status.rpm != 0;

            // Debounce +15. While cranking (slow, starter engaged) the battery sags and +15 can
            // read low, so ride through that for a while. A *running* engine gets no exemption:
            // key off must always stop it.
            if (raw) { on_ms += TICK_MS; off_ms = 0; } else { off_ms += TICK_MS; on_ms = 0; }
            bool was_on   = ign;
            bool cranking = turning && crank::status.rpm < CRANK_RPM_MAX;
            if (on_ms >= IGN_ON_MS) ign = true;
            if (off_ms >= IGN_OFF_MS && !(cranking && off_ms < CRANK_HOLD_MS)) ign = false;
            bool ign_eff = ign || bench;

            uint8_t flags = (raw ? F_IGN_INPUT : 0) | (bench ? F_BENCH_IGN : 0);

            if (ign_eff)
            {
                if (!was_on && ign) prime_ms = PRIME_MS;          // key just turned on: prime
                if (bench && status.state != IGN_ON) prime_ms = PRIME_MS;
                bool pump = turning || prime_ms > 0;
                if (prime_ms >= TICK_MS) prime_ms -= TICK_MS; else prime_ms = 0;

                relay(GPIO_PIN_7, true);
                relay(GPIO_PIN_8, pump);
                flags |= F_MAIN_RELAY | (pump ? F_FUEL_PUMP : 0);
                status.state       = IGN_ON;
                status.sleep_in_ds = 0;
                off_for_ms         = 0;
            }
            else
            {
                bool was_running = status.state != IGN_OFF;
                status.state = IGN_OFF;                    // first: stops new events being armed
                status.flags = flags;                      // main relay flag off
                if (was_running) stop_outputs();           // then cancel anything already armed
                relay(GPIO_PIN_7, false);
                relay(GPIO_PIN_8, false);
                prime_ms     = 0;
                off_for_ms  += TICK_MS;

                bool held = client_connected || update_in_progress || now_ms < BOOT_GRACE_MS;
                if (held) flags |= F_HELD_AWAKE;

                uint32_t left_ms = off_for_ms < RUNON_MS ? RUNON_MS - off_for_ms : 0;
                if (now_ms < BOOT_GRACE_MS && BOOT_GRACE_MS - now_ms > left_ms) left_ms = BOOT_GRACE_MS - now_ms;
                status.sleep_in_ds = held && left_ms == 0 ? 0 : uint16_t(left_ms / 100);

                if (!held && off_for_ms >= RUNON_MS && !turning) { status.flags = flags; sleep_now(); }
            }
            status.flags = flags;
            vTaskDelay(pdMS_TO_TICKS(TICK_MS));
        }
    },
    "Power"
);
