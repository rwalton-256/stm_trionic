// crank.cpp - crank trigger decoder on PA6 / TIM3_CH1 (hardware input capture)
#include <stm32f4xx_hal.h>
#include <cstdint>
#include "crank.hpp"
#include "outputs.hpp"
#include "tune.hpp"

namespace crank
{
    Status status{};

    static uint16_t last_capture;
    static uint32_t last_period;
    static uint32_t rev_ticks;
    static uint8_t  overflows;
    static bool     have_first_edge;

    // Internal crank simulator (TIM7): when tune "sim rpm" is non-zero, the real PA6 capture
    // is ignored and a perfect 60-2 tooth train is fed straight into the decoder.
    static uint8_t  sim_slot;
    static bool     sim_active;

    static void lose_sync()
    {
        outputs::stop();
        if (status.synced) status.sync_losses++;
        status.synced = false;
        status.tooth  = 0;
        rev_ticks     = 0;
    }

    void init()
    {
        __HAL_RCC_TIM3_CLK_ENABLE();

        GPIO_InitTypeDef g{};
        g.Pin       = GPIO_PIN_6;
        g.Mode      = GPIO_MODE_AF_PP;
        g.Pull      = GPIO_NOPULL;
        g.Speed     = GPIO_SPEED_FREQ_HIGH;
        g.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOA, &g);

        // TIM3 is on APB1: timer clock 84 MHz / 84 = 1 MHz (1 us ticks)
        TIM3->PSC   = 84 - 1;
        TIM3->ARR   = 0xFFFF;
        TIM3->CCMR1 = (1u << TIM_CCMR1_CC1S_Pos) | (0x3u << TIM_CCMR1_IC1F_Pos); // TI1, light filter
        TIM3->CCER  = TIM_CCER_CC1E;                                              // rising edge
        TIM3->DIER  = TIM_DIER_CC1IE | TIM_DIER_UIE;
        TIM3->EGR   = TIM_EGR_UG;
        TIM3->SR    = 0;

        // Priority 1 is above configMAX_SYSCALL_INTERRUPT_PRIORITY: never call FreeRTOS from here.
        HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
        TIM3->CR1 = TIM_CR1_CEN;

        // Simulator tick: TIM7 (basic timer, APB1 84 MHz -> 1 MHz), one interrupt per tooth slot.
        // Same priority as TIM3 so the two never interleave inside the decoder.
        __HAL_RCC_TIM7_CLK_ENABLE();
        TIM7->PSC  = 84 - 1;
        TIM7->ARR  = 10000 - 1;                     // idle: poll the tune every 10 ms
        TIM7->DIER = TIM_DIER_UIE;
        TIM7->EGR  = TIM_EGR_UG;
        TIM7->SR   = 0;
        HAL_NVIC_SetPriority(TIM7_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(TIM7_IRQn);
        TIM7->CR1  = TIM_CR1_ARPE | TIM_CR1_CEN;    // ARR changes take effect next period
    }

    static inline void on_tooth(uint16_t capture)
    {
        uint16_t prev   = last_capture;
        uint32_t period = uint16_t(capture - prev);   // 16-bit subtraction absorbs one wrap
        last_capture = capture;
        // Only stale if the real period could exceed 65.5 ms: two wraps, or one wrap
        // with the counter already past the previous capture.
        bool stale = overflows >= 2 || (overflows == 1 && capture >= prev);
        overflows = 0;

        if (!have_first_edge || stale)
        {
            if (have_first_edge) status.stale_edges++;
            have_first_edge = true; last_period = 0; lose_sync(); return;
        }

        bool gap = last_period && (period * 2 > last_period * 3);   // > 1.5x previous tooth

        if (gap)
        {
            if (status.synced && status.tooth != REAL_TEETH)
            {
                status.gap_mismatch++;
                lose_sync();
            }
            else
            {
                if (status.synced && rev_ticks)
                    status.rpm = (60u * TICK_HZ) / (rev_ticks + period);
                if (status.tooth == REAL_TEETH || status.synced) status.synced = true;
                status.revs++;
            }
            status.tooth = 1;
            rev_ticks    = 0;
            last_period  = period / (MISSING_TEETH + 1);   // keep the gap from skewing the next compare
            if (status.synced) outputs::on_tooth(1, capture, last_period, status.rpm);
        }
        else
        {
            if (status.tooth) status.tooth++;
            if (status.tooth > REAL_TEETH) { status.overcount++; lose_sync(); }
            rev_ticks  += period;
            last_period = period;
            if (status.synced) outputs::on_tooth(status.tooth, capture, period, status.rpm);
        }
        status.tooth_period = period;
    }
}

extern "C" void TIM3_IRQHandler()
{
    uint32_t sr = TIM3->SR & TIM3->DIER;   // only sources we enabled (capture is off while simulating)

    auto on_wrap = []()
    {
        TIM3->SR = ~TIM_SR_UIF;
        if (crank::overflows < 255) crank::overflows++;
        if (crank::overflows >= 2)               // ~131 ms with no tooth: engine stopped
        {
            if (crank::overflows == 2) crank::status.stopped++;
            crank::lose_sync();
            crank::status.rpm = 0;
        }
    };

    if (sr & TIM_SR_CC1IF)
    {
        uint16_t ccr = uint16_t(TIM3->CCR1);     // reading CCR1 clears CC1IF
        // Both pending: if the capture is in the bottom half of the count, the wrap came first.
        bool wrap_first = (sr & TIM_SR_UIF) && ccr < 0x8000;
        if (wrap_first) on_wrap();
        crank::on_tooth(ccr);
        if ((sr & TIM_SR_UIF) && !wrap_first) on_wrap();
    }
    else if (sr & TIM_SR_UIF)
    {
        on_wrap();
    }
}

extern "C" void TIM7_IRQHandler()
{
    using namespace crank;
    TIM7->SR = ~TIM_SR_UIF;

    uint32_t rpm  = tune_page1.sim_rpm;
    bool     want = rpm != 0;

    if (want != sim_active)
    {
        // Switch tooth source: real capture off while simulating, back on afterwards.
        sim_active = want;
        if (want) TIM3->DIER &= ~TIM_DIER_CC1IE;
        else    { TIM3->SR = ~TIM_SR_CC1IF; TIM3->DIER |= TIM_DIER_CC1IE; }
        have_first_edge   = false;
        lose_sync();
        sim_slot          = 0;
        status.simulating = want;
    }
    if (!want) { TIM7->ARR = 10000 - 1; return; }

    if (rpm < 50)   rpm = 50;                       // below ~46 rpm the 3-slot gap exceeds 65 ms
    if (rpm > 9000) rpm = 9000;
    TIM7->ARR = 1000000u / rpm - 1;                 // 6 deg slot, us = 1e6 / rpm

    if (sim_slot < REAL_TEETH)
    {
        // A TIM3 wrap may be pending but not yet counted (same priority, can't preempt us).
        // Count it first, or the next tooth would look stale.
        if (TIM3->SR & TIM_SR_UIF) TIM3_IRQHandler();
        on_tooth(uint16_t(TIM3->CNT));              // timestamp from the same clock as real captures
    }
    if (++sim_slot >= TOOTH_POSITIONS) sim_slot = 0;
}