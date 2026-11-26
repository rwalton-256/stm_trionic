// outputs.cpp - injector and spark scheduling (see outputs.hpp for pin map)
//
// How it works: on each crank tooth, every event whose angle falls before the next tooth
// is armed on its timer channel as an output compare. The hardware sets the pin at exactly
// the right microsecond; the compare interrupt then programs the end of the pulse, which the
// hardware also clears on time. Interrupt latency only has to be shorter than the pulse.
//
// All three timers (TIM3 crank, TIM4, TIM1) tick at 1 MHz. Delays are converted to each
// timer's own counter at arm time, so the timers don't need to be synchronised.
#include <stm32f4xx_hal.h>
#include "outputs.hpp"
#include "crank.hpp"
#include "tune.hpp"
#include "power.hpp"
#include "fuel.hpp"

namespace outputs
{
    Status status{};

    enum : uint32_t { OC_ACTIVE_ON_MATCH = 1, OC_INACTIVE_ON_MATCH = 2, OC_FORCE_INACTIVE = 4 };
    enum Phase : uint8_t { IDLE, ARMED, ON };

    struct Channel
    {
        TIM_TypeDef*     tim;
        uint8_t          ch;      // 0..3
        volatile uint8_t phase;
        uint16_t         width;   // pulse length for the current event, us
    };

    // Index = cylinder - 1
    static Channel inj[4] = { { TIM4, 0, IDLE, 0 }, { TIM4, 1, IDLE, 0 }, { TIM4, 2, IDLE, 0 }, { TIM4, 3, IDLE, 0 } };
    static Channel ign[4] = { { TIM1, 0, IDLE, 0 }, { TIM1, 1, IDLE, 0 }, { TIM1, 2, IDLE, 0 }, { TIM1, 3, IDLE, 0 } };

    // Firing order 1-3-4-2, wasted spark: pair A = cylinders 1+4 at 0 deg, pair B = 3+2 at 180 deg.
    struct Pair { uint8_t a, b; uint16_t tdc_deg; };
    static constexpr Pair PAIRS[2] = { { 0, 3, 0 }, { 2, 1, 180 } };

    static uint32_t last_spark_end_cyc;   // DWT cycle count when the last trigger was released
    static bool     spark_ever_fired;

    // ---- register helpers -------------------------------------------------------------
    static volatile uint32_t& ccr(const Channel& c) { return (&c.tim->CCR1)[c.ch]; }

    static void set_mode(const Channel& c, uint32_t mode)
    {
        volatile uint32_t& r = c.ch < 2 ? c.tim->CCMR1 : c.tim->CCMR2;
        uint32_t sh = (c.ch & 1) ? 12 : 4;                 // OCxM field
        r = (r & ~(7u << sh)) | (mode << sh);
    }

    static void irq_enable(const Channel& c, bool on)
    {
        uint32_t bit = TIM_DIER_CC1IE << c.ch;
        if (on) c.tim->DIER |= bit; else c.tim->DIER &= ~bit;
    }

    static void force_off(Channel& c)
    {
        irq_enable(c, false);
        set_mode(c, OC_FORCE_INACTIVE);
        c.phase = IDLE;
    }

    // Arm a pulse starting `delay` us from now, `width` us long.
    static bool arm(Channel& c, uint32_t delay, uint16_t width)
    {
        if (c.phase != IDLE) { status.overlaps++; return false; }
        if (delay < 5) delay = 5;                          // must land in the future, safely
        c.width = width;
        ccr(c) = uint16_t(c.tim->CNT + delay);
        c.tim->SR = ~(TIM_SR_CC1IF << c.ch);
        set_mode(c, OC_ACTIVE_ON_MATCH);
        c.phase = ARMED;
        irq_enable(c, true);
        return true;
    }

    // Compare interrupt for one channel: start edge -> program the end; end edge -> done.
    static void on_compare(Channel& c, bool is_spark)
    {
        if (c.phase == ARMED)
        {
            uint16_t start = uint16_t(ccr(c));
            ccr(c) = uint16_t(start + c.width);
            set_mode(c, OC_INACTIVE_ON_MATCH);
            c.phase = ON;
            // If we were too slow and the end time already passed, end it now rather than
            // leaving the output on for a whole 65 ms timer wrap.
            if (uint16_t(c.tim->CNT - start) >= c.width) { force_off(c); goto ended; }
            return;
        }
        if (c.phase == ON)
        {
            force_off(c);
            goto ended;
        }
        irq_enable(c, false);
        return;
    ended:
        if (is_spark) { last_spark_end_cyc = DWT->CYCCNT; spark_ever_fired = true; status.spark_pulses++; }
        else          { status.inj_pulses++; }
    }

    static void timer_init(TIM_TypeDef* t, uint32_t psc)
    {
        t->CR1   = 0;
        t->PSC   = psc;
        t->ARR   = 0xFFFF;
        t->CCMR1 = (OC_FORCE_INACTIVE << 4) | (OC_FORCE_INACTIVE << 12);   // no preload: CCR writes act at once
        t->CCMR2 = (OC_FORCE_INACTIVE << 4) | (OC_FORCE_INACTIVE << 12);
        t->CCER  = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;  // active high
        t->DIER  = 0;
        t->EGR   = TIM_EGR_UG;
        t->SR    = 0;
    }

    void init()
    {
        // Cycle counter for the DI recharge guard
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;

        __HAL_RCC_TIM4_CLK_ENABLE();
        __HAL_RCC_TIM1_CLK_ENABLE();
        timer_init(TIM4, 84 - 1);                          // APB1 timer clock 84 MHz -> 1 MHz
        timer_init(TIM1, 168 - 1);                         // APB2 timer clock 168 MHz -> 1 MHz
        TIM1->BDTR = TIM_BDTR_MOE;                         // advanced timer: main output enable

        // Outputs are forced low before the pins are handed to the timers.
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        GPIO_InitTypeDef g{};
        g.Mode  = GPIO_MODE_AF_PP;
        g.Pull  = GPIO_PULLDOWN;
        g.Speed = GPIO_SPEED_FREQ_MEDIUM;
        g.Pin       = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        g.Alternate = GPIO_AF2_TIM4;
        HAL_GPIO_Init(GPIOD, &g);
        g.Pin       = GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
        g.Alternate = GPIO_AF1_TIM1;
        HAL_GPIO_Init(GPIOE, &g);

        // Same priority as the crank interrupt, so none of them can preempt an arm() in progress.
        HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
        HAL_NVIC_SetPriority(TIM1_CC_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
        HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);

        TIM4->CR1 = TIM_CR1_CEN;
        TIM1->CR1 = TIM_CR1_CEN;
        stop();                                          // start from a known state
    }

    void stop()
    {
        for (auto& c : inj) force_off(c);
        for (auto& c : ign) force_off(c);
        spark_ever_fired = false;     // the cycle counter may wrap (25 s) before the next spark
    }

    // Degrees from `from` forward to `to`, in a 360 deg cycle.
    static inline float forward(float from, float to)
    {
        float d = to - from;
        while (d < 0.0f)    d += 360.0f;
        while (d >= 360.0f) d -= 360.0f;
        return d;
    }

    void on_tooth(uint8_t tooth, uint16_t capture, uint32_t period, uint32_t rpm)
    {
        const TunePage1& t = tune_page1;

        // Nothing fires without ignition on and the main relay powering injectors and cassette.
        if (!power::outputs_allowed()) return;

        // Rev limiter: hard cut with 200 rpm hysteresis
        uint32_t limit = t.rev_limit ? t.rev_limit : 6000;
        if (rpm > limit)              status.rev_limited = true;
        else if (rpm + 200 < limit)   status.rev_limited = false;
        if (status.rev_limited || period == 0) return;

        bool fuel  = t.outputs_enable & 1;
        bool spark = t.outputs_enable & 2;
        if (!fuel && !spark) return;

        // Angle of this tooth after cylinder 1 TDC. Tooth 1 is `trigger_angle` deg before TDC.
        float tooth_deg = forward(0.0f, float(tooth - 1) * 6.0f - float(t.trigger_angle));
        float window    = (tooth == crank::REAL_TEETH) ? 6.0f * (crank::MISSING_TEETH + 1) : 6.0f;
        float us_per_deg = float(period) / 6.0f;

        // Time already gone since the tooth edge (we're a few us into the interrupt)
        uint32_t elapsed = uint16_t(TIM3->CNT - capture);

        for (const Pair& p : PAIRS)
        {
            // ---- spark: pair fires together (each coil gets half the stored energy) ----
            if (spark)
            {
                float d = forward(tooth_deg, float(p.tdc_deg) - float(int8_t(t.advance)));
                if (d < window)
                {
                    uint32_t delay = uint32_t(d * us_per_deg);
                    delay = delay > elapsed ? delay - elapsed : 0;
                    // DI cassette: >= 2.5 ms recharge after the trigger is released
                    uint32_t since = (DWT->CYCCNT - last_spark_end_cyc) / 168u;
                    if (spark_ever_fired && since + delay < 2500u)
                    {
                        status.spark_skipped++;
                    }
                    else
                    {
                        uint16_t w = t.spark_pulse_us;
                        if (w < 500)  w = 500;             // cassette needs >= ~500 us low
                        if (w > 2000) w = 2000;
                        bool a = arm(ign[p.a], delay, w);
                        bool b = arm(ign[p.b], delay, w);
                        if (a || b) status.sparks++;
                    }
                }
            }

            // ---- fuel: both injectors of the pair, once per revolution ----
            uint32_t pw_cmd = t.fuel_mode == 1 ? fuel::pulse_us() : t.inj_pw_us;
            if (fuel && pw_cmd >= 100)
            {
                float d = forward(tooth_deg, float(p.tdc_deg) - float(t.inj_angle));
                if (d < window)
                {
                    uint32_t delay = uint32_t(d * us_per_deg);
                    delay = delay > elapsed ? delay - elapsed : 0;
                    // Never longer than 85% of a revolution (or the 16-bit timer)
                    uint32_t rev_us = period * crank::TOOTH_POSITIONS;
                    uint32_t w = pw_cmd;
                    if (w > rev_us * 85 / 100) w = rev_us * 85 / 100;
                    if (w > 60000) w = 60000;
                    bool a = arm(inj[p.a], delay, uint16_t(w));
                    bool b = arm(inj[p.b], delay, uint16_t(w));
                    if (a || b) status.injections++;
                }
            }
        }
    }

    static void service(TIM_TypeDef* tim, Channel* chans, bool is_spark)
    {
        uint32_t sr = tim->SR & tim->DIER;
        for (int i = 0; i < 4; i++)
        {
            uint32_t flag = TIM_SR_CC1IF << i;
            if (sr & flag)
            {
                tim->SR = ~flag;
                on_compare(chans[i], is_spark);
            }
        }
    }
}

extern "C" void TIM4_IRQHandler()    { outputs::service(TIM4, outputs::inj, false); }
extern "C" void TIM1_CC_IRQHandler() { outputs::service(TIM1, outputs::ign, true); }
