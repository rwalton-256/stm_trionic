#include "periph.hpp"
#include "stm32f4xx_hal.h"
#include <cstring>

// ---- the fake register blocks the firmware sees ----
TIM_TypeDef    sim_tim1{}, sim_tim3{}, sim_tim4{}, sim_tim7{};
DWT_Type       sim_dwt{};
CoreDebug_Type sim_coredebug{};
GPIO_TypeDef   sim_gpioa{0,0,0,'A'}, sim_gpiob{0,0,0,'B'}, sim_gpioc{0,0,0,'C'}, sim_gpiod{0,0,0,'D'},
               sim_gpioe{0,0,0,'E'}, sim_gpiof{0,0,0,'F'}, sim_gpiog{0,0,0,'G'};

void HAL_GPIO_Init(GPIO_TypeDef*, GPIO_InitTypeDef*) {}
void HAL_GPIO_WritePin(GPIO_TypeDef* g, uint16_t pin, GPIO_PinState s)
{
    if (s) g->ODR |= pin; else g->ODR &= ~uint32_t(pin);
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* g, uint16_t pin) { return (g->IDR & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET; }

namespace sim
{
    static uint64_t          t = 0;
    static std::vector<Edge> log;
    static bool              pin_state[2][4];   // [spark/inj][ch]

    struct OutTimer { TIM_TypeDef* tim; char name; int idx; void (*isr)(); };
    static const OutTimer OUT[2] = { { &sim_tim1, '1', 0, TIM1_CC_IRQHandler },
                                     { &sim_tim4, '4', 1, TIM4_IRQHandler } };

    static uint32_t mode(TIM_TypeDef* tim, int ch)
    {
        uint32_t r  = ch < 2 ? tim->CCMR1 : tim->CCMR2;
        return (r >> ((ch & 1) ? 12 : 4)) & 7u;
    }
    static volatile uint32_t& ccr(TIM_TypeDef* tim, int ch) { return (&tim->CCR1)[ch]; }

    static void set_pin(const OutTimer& o, int ch, bool level)
    {
        if (pin_state[o.idx][ch] == level) return;
        pin_state[o.idx][ch] = level;
        log.push_back({ t, o.name, uint8_t(ch), level });
    }

    // Forced output modes act immediately (no match needed).
    static void apply_forced()
    {
        for (const auto& o : OUT)
            for (int ch = 0; ch < 4; ch++)
            {
                uint32_t m = mode(o.tim, ch);
                if (m == 4) set_pin(o, ch, false);
                if (m == 5) set_pin(o, ch, true);
            }
    }

    static void sync_counters()
    {
        uint16_t c = uint16_t(t);           // every timer: 1 MHz, free-running, same phase
        sim_tim1.CNT = sim_tim3.CNT = sim_tim4.CNT = c;
        sim_dwt.CYCCNT = uint32_t(t * 168u);
    }

    void reset()
    {
        t = 0; log.clear(); std::memset(pin_state, 0, sizeof(pin_state));
        TIM_TypeDef* all[] = { &sim_tim1, &sim_tim3, &sim_tim4, &sim_tim7 };
        for (auto* x : all) *x = TIM_TypeDef{};
        sync_counters();
    }

    uint64_t now() { return t; }
    std::vector<Edge>& edges() { return log; }

    // Time until a compare channel next matches (1..65536 us), or 0 if it can't fire.
    static uint64_t until_match(TIM_TypeDef* tim, int ch)
    {
        uint32_t m = mode(tim, ch);
        bool active = (m == 1 || m == 2) || (tim->DIER & (TIM_DIER_CC1IE << ch));
        if (!active || !(tim->CR1 & TIM_CR1_CEN)) return 0;
        uint16_t d = uint16_t(ccr(tim, ch) - uint16_t(t));
        return d ? d : 65536;
    }

    static void fire_match(const OutTimer& o, int ch)
    {
        uint32_t m = mode(o.tim, ch);
        if (m == 1) set_pin(o, ch, true);
        if (m == 2) set_pin(o, ch, false);
        o.tim->SR.hw_set(TIM_SR_CC1IF << ch);
        if (o.tim->DIER & (TIM_DIER_CC1IE << ch)) { o.isr(); apply_forced(); }
    }

    void advance_to(uint64_t target)
    {
        apply_forced();
        while (t < target)
        {
            // Next event: a compare match on TIM1/TIM4, or the TIM3 wrap.
            uint64_t step = target - t;
            uint64_t wrap = 65536 - (t & 0xFFFF);
            if (wrap < step) step = wrap;
            for (const auto& o : OUT)
                for (int ch = 0; ch < 4; ch++)
                {
                    uint64_t u = until_match(o.tim, ch);
                    if (u && u < step) step = u;
                }
            t += step;
            sync_counters();

            for (const auto& o : OUT)
                for (int ch = 0; ch < 4; ch++)
                    if (until_match(o.tim, ch) == 65536 && (mode(o.tim, ch) == 1 || mode(o.tim, ch) == 2
                        || (o.tim->DIER & (TIM_DIER_CC1IE << ch))))
                        fire_match(o, ch);

            if ((t & 0xFFFF) == 0)
            {
                sim_tim3.SR.hw_set(TIM_SR_UIF);
                if (sim_tim3.DIER & TIM_DIER_UIE) { TIM3_IRQHandler(); apply_forced(); }
            }
        }
    }

    void crank_edge()
    {
        sim_tim3.CCR1 = uint16_t(t);
        sim_tim3.SR.hw_set(TIM_SR_CC1IF);
        if (sim_tim3.DIER & TIM_DIER_CC1IE) { TIM3_IRQHandler(); apply_forced(); }
        sim_tim3.SR.hw_clear(TIM_SR_CC1IF);  // reading CCR1 clears it on hardware
    }
}
