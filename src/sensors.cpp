// sensors.cpp - slow analog inputs, one task, polled conversions every 100 ms
//
//   ADC1: internal temperature sensor (ch16) and VREFINT (ch17)
//   ADC3: shield inputs (Arduino A0..A5 on the Olimex)
//
// Calibration values all come from tune page 1 (Settings -> Sensor calibration in TunerStudio).
//
// HAL must be included before FreeRTOS.hpp (SystemCoreClock linkage clash in FreeRTOSConfig.h).
#include <stm32f4xx_hal.h>
#include <FreeRTOS.hpp>
#include <initializer_list>
#include <cmath>
#include "sensors.hpp"
#include "tune.hpp"

namespace sensors
{
    Readings readings{};

    struct Channel { GPIO_TypeDef* port; uint16_t pin; uint8_t adc_ch; };

    // Order must match enum Input. All on ADC3.
    static const Channel CHANNELS[INPUT_COUNT] =
    {
        { GPIOC, GPIO_PIN_0,  10 },   // IAT       A0  PC0  ADC123_IN10  (1k pull-up R40)
        { GPIOF, GPIO_PIN_6,   4 },   // CLT       A1  PF6  ADC3_IN4     (1k pull-up R47)
        { GPIOF, GPIO_PIN_7,   5 },   // TPS       A2  PF7  ADC3_IN5     (10k/20k, R41/R45)
        { GPIOF, GPIO_PIN_8,   6 },   // MAP       A3  PF8  ADC3_IN6     (10k/20k, R37/R38)
        { GPIOF, GPIO_PIN_9,   7 },   // BATTERY   A4  PF9  ADC3_IN7     (41.2k/10k, R53/R55)
        { GPIOF, GPIO_PIN_10,  8 },   // WIDEBAND  A5  PF10 ADC3_IN8     (10k/20k, R48/R50)
    };

    // Factory calibration (STM32F40x system memory), measured at VDDA = 3.3 V
    static const uint16_t& VREFINT_CAL = *reinterpret_cast<const uint16_t*>(0x1FFF7A2A);
    static const uint16_t& TS_CAL1     = *reinterpret_cast<const uint16_t*>(0x1FFF7A2C); // 30 C
    static const uint16_t& TS_CAL2     = *reinterpret_cast<const uint16_t*>(0x1FFF7A2E); // 110 C

    static constexpr int SAMPLES = 16;

    // Clamp a tune value to a sane range, falling back to a default if it's nonsense.
    static float setting(uint32_t v, uint32_t lo, uint32_t hi, uint32_t fallback)
    {
        return float((v < lo || v > hi) ? fallback : v);
    }

    static void init()
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();
        for (const auto& c : CHANNELS)
        {
            GPIO_InitTypeDef g{};
            g.Pin  = c.pin;
            g.Mode = GPIO_MODE_ANALOG;
            g.Pull = GPIO_NOPULL;
            HAL_GPIO_Init(c.port, &g);
        }

        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_ADC3_CLK_ENABLE();

        // Common: ADC clock = PCLK2 (84 MHz) / 4 = 21 MHz; enable temp sensor + VREFINT
        ADC->CCR = (1u << ADC_CCR_ADCPRE_Pos) | ADC_CCR_TSVREFE;

        // 480-cycle sampling (22.9 us) everywhere: internal channels need >= 10 us,
        // and the shield dividers are high impedance.
        for (ADC_TypeDef* adc : { ADC1, ADC3 })
        {
            adc->CR1   = 0;                // 12-bit, single conversion
            adc->CR2   = 0;
            adc->SMPR1 = 0x07FFFFFF;       // ch10..18 = 480 cycles
            adc->SMPR2 = 0x3FFFFFFF;       // ch0..9   = 480 cycles
            adc->SQR1  = 0;                // sequence length 1
            adc->CR2   = ADC_CR2_ADON;
        }
    }

    static uint32_t average(ADC_TypeDef* adc, uint8_t channel)
    {
        adc->SQR3 = channel;
        uint32_t sum = 0;
        for (int i = 0; i < SAMPLES; i++)
        {
            adc->SR   = 0;
            adc->CR2 |= ADC_CR2_SWSTART;
            while (!(adc->SR & ADC_SR_EOC)) {}
            sum += adc->DR;
        }
        return sum / SAMPLES;
    }

    static int16_t die_temp(uint32_t ts_raw, uint32_t vref_raw)
    {
        bool cal_ok = TS_CAL1 > 0 && TS_CAL2 > TS_CAL1 && TS_CAL2 < 4095
                   && VREFINT_CAL > 1000 && VREFINT_CAL < 2000;
        if (cal_ok && vref_raw)
        {
            float ts = float(ts_raw) * float(VREFINT_CAL) / float(vref_raw);
            float t  = 30.0f + (ts - float(TS_CAL1)) * 80.0f / float(TS_CAL2 - TS_CAL1);
            return int16_t(t * 10.0f);
        }
        float v = float(ts_raw) * 3.3f / 4095.0f;   // datasheet typicals: 0.76 V @ 25 C, 2.5 mV/C
        return int16_t(((v - 0.76f) / 0.0025f + 25.0f) * 10.0f);
    }

    // NTC thermistor with a pull-up to VDDA, beta model. Returns C x10.
    // Open circuit reads -40 C, short reads +150 C, so faults are obvious on a gauge.
    static int16_t thermistor(float pin_mv, float vdda_mv, const TunePage1& t)
    {
        float r_pu = setting(t.therm_pullup_ohm, 100, 10000, 1000);
        float r25  = setting(t.therm_r25_ohm,    100, 60000, 2050);
        float beta = setting(t.therm_beta,      1000,  6000, 3500);

        if (pin_mv >= vdda_mv - 5.0f) return -400;
        if (pin_mv <= 5.0f)           return 1500;
        float r = r_pu * pin_mv / (vdda_mv - pin_mv);
        float k = 1.0f / (1.0f / 298.15f + logf(r / r25) / beta);
        float c = k - 273.15f;
        if (c < -40.0f) c = -40.0f;
        if (c > 150.0f) c = 150.0f;
        return int16_t(c * 10.0f);
    }

    // Linear interpolation from sensor voltage (0..5000 mV) to a value.
    static float linear(float mv, float at0, float at5)
    {
        return at0 + (at5 - at0) * mv / 5000.0f;
    }

    static void sample_all()
    {
        const TunePage1& t = tune_page1;

        // Internal channels first: VREFINT gives the real VDDA for scaling everything else.
        uint32_t vref = average(ADC1, 17);
        uint32_t ts   = average(ADC1, 16);
        readings.mcu_temp_deci_c = die_temp(ts, vref);

        uint32_t vdda = 3300;
        if (vref && VREFINT_CAL > 1000 && VREFINT_CAL < 2000)
            vdda = 3300u * VREFINT_CAL / vref;
        readings.vdda_mv = uint16_t(vdda);

        for (uint8_t i = 0; i < INPUT_COUNT; i++)
            readings.pin_mv[i] = uint16_t(average(ADC3, CHANNELS[i].adc_ch) * vdda / 4095);

        // Battery
        float batt_ratio = setting(t.batt_ratio_x1000, 1000, 20000, 5120) / 1000.0f;
        readings.battery_mv = uint16_t(readings.pin_mv[BATTERY] * batt_ratio);

        // Thermistors
        readings.iat_deci_c = thermistor(readings.pin_mv[IAT], float(vdda), t);
        readings.clt_deci_c = thermistor(readings.pin_mv[CLT], float(vdda), t);

        // 5 V sensors: undo the 10k/20k divider to get the voltage at the sensor
        float in5v = setting(t.in5v_ratio_x1000, 1000, 5000, 1500) / 1000.0f;
        float tps_mv = readings.pin_mv[TPS]      * in5v;
        float map_mv = readings.pin_mv[MAP]      * in5v;
        float wbo_mv = readings.pin_mv[WIDEBAND] * in5v;

        // TPS: percent between calibrated closed and wide-open voltages
        readings.tps_sensor_mv = uint16_t(tps_mv);
        float lo = setting(t.tps_min_mv, 0, 5000, 200);
        float hi = setting(t.tps_max_mv, 0, 5000, 4000);
        float pct = (hi > lo + 100.0f) ? (tps_mv - lo) * 100.0f / (hi - lo) : 0.0f;
        if (pct < 0.0f)   pct = 0.0f;
        if (pct > 100.0f) pct = 100.0f;
        readings.tps_x10 = uint16_t(pct * 10.0f);

        // MAP: linear, kPa at 0 V and 5 V (stock Saab 2.5 bar: 11.3 .. 261.3 kPa)
        float kpa = linear(map_mv, t.map_kpa0_x10 / 10.0f, t.map_kpa5_x10 / 10.0f);
        readings.map_kpa_x10 = uint16_t(kpa < 0.0f ? 0.0f : kpa * 10.0f);

        // Wideband: linear, AFR at 0 V and 5 V (depends on the controller)
        float afr = linear(wbo_mv, t.afr0_x10 / 10.0f, t.afr5_x10 / 10.0f);
        readings.afr_x10 = uint16_t(afr < 0.0f ? 0.0f : afr * 10.0f);
    }
}

static FreeRTOS::task<
    512,
    tskIDLE_PRIORITY + 1
> _sensors_task(
    [](void*)
    {
        sensors::init();
        vTaskDelay(pdMS_TO_TICKS(10));                 // temp sensor / ADC start-up

        for (;;)
        {
            sensors::sample_all();                     // 8 channels x 16 samples x ~23 us = ~3 ms
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    },
    "Sensors"
);