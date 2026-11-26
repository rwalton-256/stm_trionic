// Fake STM32F4 HAL/CMSIS for host simulation. Peripherals are plain structs in RAM;
// sim/model/periph.cpp advances them and calls the firmware's interrupt handlers.
#pragma once
#include <cstdint>

// ---------------- timers ----------------
// SR is "rc_w0" on hardware: writing 0 clears a flag, writing 1 leaves it alone.
// `TIMx->SR = ~FLAG` must clear only FLAG, so SR can't be a plain integer.
struct RcW0
{
    volatile uint32_t v = 0;
    RcW0& operator=(uint32_t w) { v = v & w; return *this; }   // firmware write
    operator uint32_t() const   { return v; }
    void  hw_set(uint32_t bits) { v = v | bits; }              // peripheral model only
    void  hw_clear(uint32_t bits) { v = v & ~bits; }
};
struct TIM_TypeDef
{
    volatile uint32_t CR1, CR2, SMCR, DIER;
    RcW0              SR;
    volatile uint32_t EGR, CCMR1, CCMR2, CCER, CNT, PSC, ARR, RCR,
                      CCR1, CCR2, CCR3, CCR4, BDTR, DCR, DMAR;
};
extern TIM_TypeDef sim_tim1, sim_tim3, sim_tim4, sim_tim7;
#define TIM1 (&sim_tim1)
#define TIM3 (&sim_tim3)
#define TIM4 (&sim_tim4)
#define TIM7 (&sim_tim7)

#define TIM_CR1_CEN          (1u << 0)
#define TIM_CR1_ARPE         (1u << 7)
#define TIM_DIER_UIE         (1u << 0)
#define TIM_DIER_CC1IE       (1u << 1)
#define TIM_SR_UIF           (1u << 0)
#define TIM_SR_CC1IF         (1u << 1)
#define TIM_EGR_UG           (1u << 0)
#define TIM_CCER_CC1E        (1u << 0)
#define TIM_CCER_CC2E        (1u << 4)
#define TIM_CCER_CC3E        (1u << 8)
#define TIM_CCER_CC4E        (1u << 12)
#define TIM_CCMR1_CC1S_Pos   0
#define TIM_CCMR1_IC1F_Pos   4
#define TIM_BDTR_MOE         (1u << 15)

// ---------------- cycle counter ----------------
struct DWT_Type       { volatile uint32_t CTRL, CYCCNT; };
struct CoreDebug_Type { volatile uint32_t DEMCR; };
extern DWT_Type sim_dwt; extern CoreDebug_Type sim_coredebug;
#define DWT       (&sim_dwt)
#define CoreDebug (&sim_coredebug)
#define DWT_CTRL_CYCCNTENA_Msk     (1u << 0)
#define CoreDebug_DEMCR_TRCENA_Msk (1u << 24)

// ---------------- GPIO ----------------
struct GPIO_TypeDef { volatile uint32_t MODER, ODR, IDR; char name; };
extern GPIO_TypeDef sim_gpioa, sim_gpiob, sim_gpioc, sim_gpiod, sim_gpioe, sim_gpiof, sim_gpiog;
#define GPIOA (&sim_gpioa)
#define GPIOB (&sim_gpiob)
#define GPIOC (&sim_gpioc)
#define GPIOD (&sim_gpiod)
#define GPIOE (&sim_gpioe)
#define GPIOF (&sim_gpiof)
#define GPIOG (&sim_gpiog)
struct GPIO_InitTypeDef { uint32_t Pin, Mode, Pull, Speed, Alternate; };
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
void          HAL_GPIO_Init(GPIO_TypeDef*, GPIO_InitTypeDef*);
void          HAL_GPIO_WritePin(GPIO_TypeDef*, uint16_t, GPIO_PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef*, uint16_t);
#define GPIO_MODE_INPUT        0
#define GPIO_MODE_OUTPUT_PP    1
#define GPIO_MODE_AF_PP        2
#define GPIO_MODE_ANALOG       3
#define GPIO_NOPULL            0
#define GPIO_PULLUP            1
#define GPIO_PULLDOWN          2
#define GPIO_SPEED_FREQ_LOW       0
#define GPIO_SPEED_FREQ_MEDIUM    1
#define GPIO_SPEED_FREQ_HIGH      2
#define GPIO_SPEED_FREQ_VERY_HIGH 3
#define GPIO_AF1_TIM1 1
#define GPIO_AF2_TIM3 2
#define GPIO_AF2_TIM4 2
#define GPIO_PIN_0  (1u << 0)
#define GPIO_PIN_1  (1u << 1)
#define GPIO_PIN_2  (1u << 2)
#define GPIO_PIN_3  (1u << 3)
#define GPIO_PIN_4  (1u << 4)
#define GPIO_PIN_5  (1u << 5)
#define GPIO_PIN_6  (1u << 6)
#define GPIO_PIN_7  (1u << 7)
#define GPIO_PIN_8  (1u << 8)
#define GPIO_PIN_9  (1u << 9)
#define GPIO_PIN_10 (1u << 10)
#define GPIO_PIN_11 (1u << 11)
#define GPIO_PIN_12 (1u << 12)
#define GPIO_PIN_13 (1u << 13)
#define GPIO_PIN_14 (1u << 14)
#define GPIO_PIN_15 (1u << 15)

// ---------------- NVIC / clocks (no-ops) ----------------
enum IRQn_Type { TIM1_CC_IRQn, TIM3_IRQn, TIM4_IRQn, TIM7_IRQn, EXTI15_10_IRQn };
inline void HAL_NVIC_SetPriority(IRQn_Type, uint32_t, uint32_t) {}
inline void HAL_NVIC_EnableIRQ(IRQn_Type) {}
inline void __disable_irq() {}
inline void __enable_irq() {}
#define __HAL_RCC_TIM1_CLK_ENABLE()
#define __HAL_RCC_TIM3_CLK_ENABLE()
#define __HAL_RCC_TIM4_CLK_ENABLE()
#define __HAL_RCC_TIM7_CLK_ENABLE()
#define __HAL_RCC_GPIOA_CLK_ENABLE()
#define __HAL_RCC_GPIOC_CLK_ENABLE()
#define __HAL_RCC_GPIOD_CLK_ENABLE()
#define __HAL_RCC_GPIOE_CLK_ENABLE()
#define __HAL_RCC_GPIOF_CLK_ENABLE()
#define __HAL_RCC_GPIOG_CLK_ENABLE()

// Firmware interrupt handlers the model calls
extern "C" void TIM1_CC_IRQHandler();
extern "C" void TIM3_IRQHandler();
extern "C" void TIM4_IRQHandler();
extern "C" void TIM7_IRQHandler();

// ---------------- power / sleep (power.cpp) ----------------
struct ETH_TypeDef    { volatile uint32_t MACMIIAR, MACMIIDR; };
struct SYSCFG_TypeDef { volatile uint32_t EXTICR[4]; };
struct EXTI_TypeDef   { volatile uint32_t IMR, EMR, RTSR, FTSR, SWIER, PR; };
struct SysTick_Type   { volatile uint32_t CTRL; };
extern ETH_TypeDef sim_eth; extern SYSCFG_TypeDef sim_syscfg; extern EXTI_TypeDef sim_exti; extern SysTick_Type sim_systick;
#define ETH     (&sim_eth)
#define SYSCFG  (&sim_syscfg)
#define EXTI    (&sim_exti)
#define SysTick (&sim_systick)
#define ETH_MACMIIAR_CR (7u << 2)
#define ETH_MACMIIAR_MW (1u << 1)
#define ETH_MACMIIAR_MB (1u << 0)
#define SYSCFG_EXTICR4_EXTI12    (0xFu)
#define SYSCFG_EXTICR4_EXTI12_PG (6u)
#define PWR_LOWPOWERREGULATOR_ON 1
#define PWR_STOPENTRY_WFE        2
#define __HAL_RCC_SYSCFG_CLK_ENABLE()
void sim_enter_stop();          // model hook: STOP mode requested
void sim_system_reset();        // model hook: NVIC_SystemReset
inline void HAL_PWR_EnterSTOPMode(uint32_t, uint8_t) { sim_enter_stop(); }
[[noreturn]] inline void NVIC_SystemReset() { sim_system_reset(); for (;;) {} }
