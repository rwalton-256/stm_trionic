// esp_bench - bench helper for the stm_trionic shield, on an ESP-12F devkit v3 (NodeMCU)
//
//   A0  analog probe. The devkit has a 220k/100k divider on A0, so the range is 0..~3.2 V.
//       NEVER put more than 3.3 V on A0 (no 12 V, no 5 V).
//   D5  (GPIO14) crank trigger output, 3.3 V logic: wire to the Olimex PA6 / shield D12 net.
//   GND must be common with the shield.
//
// Serial commands (115200, newline-terminated):
//   adc            toggle streaming of A0 readings (5 per second)
//   ratio 5.12     divider ratio used for the "battery" column (default 5.12 = 41.2k/10k)
//   rpm 1000       start the crank generator at 1000 rpm (0 or "off" stops it)
//   teeth 60 2     tooth positions per rev, missing teeth (default 60 2)
#include <Arduino.h>
#include <ESP8266WiFi.h>

static constexpr uint8_t  CRANK_PIN   = 14;       // D5
static constexpr uint32_t ADC_FULL_MV = 3200;     // devkit A0 divider: 1023 counts ~= 3.2 V

static volatile uint8_t  tooth_positions = 60;
static volatile uint8_t  missing_teeth   = 2;
static volatile uint16_t half_index      = 0;     // half-slot counter within one revolution
static bool     streaming = true;
static float    ratio     = 5.12f;
static uint32_t rpm       = 0;

// Each tooth slot is two half-periods: high then low. Missing slots stay low.
void IRAM_ATTR crank_isr()
{
    uint16_t slot   = half_index >> 1;
    bool     first  = !(half_index & 1);
    bool     real   = slot < uint16_t(tooth_positions - missing_teeth);
    if (first && real) GPOS = (1u << CRANK_PIN);
    else               GPOC = (1u << CRANK_PIN);
    if (++half_index >= uint16_t(tooth_positions) * 2) half_index = 0;
}

static void set_rpm(uint32_t r)
{
    rpm = r;
    if (r == 0)
    {
        timer1_disable();
        GPOC = (1u << CRANK_PIN);
        Serial.println("crank: off");
        return;
    }
    // timer1 at 80 MHz / 16 = 5 MHz. Half-slot period = 60 s / (rpm * slots * 2).
    uint32_t ticks = uint32_t(5000000ULL * 60ULL / (uint64_t(r) * tooth_positions * 2));
    if (ticks < 50)        ticks = 50;            // ISR can't keep up beyond this
    if (ticks > 0x7FFFFF)  ticks = 0x7FFFFF;      // 23-bit timer
    timer1_attachInterrupt(crank_isr);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
    timer1_write(ticks);
    Serial.printf("crank: %u rpm, %u-%u, %.1f us per tooth\n",
                  unsigned(r), tooth_positions, missing_teeth, ticks * 2 / 5.0f);
}

static void handle_line(String line)
{
    line.trim();
    if (line == "adc")                    { streaming = !streaming; }
    else if (line.startsWith("ratio "))   { ratio = line.substring(6).toFloat(); Serial.printf("ratio %.3f\n", ratio); }
    else if (line == "off")               { set_rpm(0); }
    else if (line.startsWith("rpm "))     { set_rpm(line.substring(4).toInt()); }
    else if (line.startsWith("teeth "))
    {
        int a = 0, b = 0;
        if (sscanf(line.c_str() + 6, "%d %d", &a, &b) == 2 && a > b && a <= 120 && b >= 0)
        {
            tooth_positions = a; missing_teeth = b; half_index = 0;
            if (rpm) set_rpm(rpm);
            Serial.printf("teeth %d-%d\n", a, b);
        }
    }
    else if (line.length()) Serial.println("commands: adc | ratio <x> | rpm <n> | off | teeth <n> <missing>");
}

void setup()
{
    WiFi.mode(WIFI_OFF);                          // WiFi activity adds noise to the ADC
    WiFi.forceSleepBegin();
    pinMode(CRANK_PIN, OUTPUT);
    digitalWrite(CRANK_PIN, LOW);
    Serial.begin(115200);
    delay(100);
    Serial.println("\nesp_bench ready. 'adc' toggles probe output, 'rpm 1000' starts the crank signal.");
}

void loop()
{
    static String line;
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r') { if (line.length()) handle_line(line); line = ""; }
        else line += c;
    }

    static uint32_t last = 0;
    if (streaming && millis() - last >= 200)
    {
        last = millis();
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) sum += analogRead(A0);
        uint32_t raw = sum / 16;
        uint32_t mv  = raw * ADC_FULL_MV / 1023;
        Serial.printf("A0 raw %4u  pin %4u mV  battery %5.2f V\n", unsigned(raw), unsigned(mv), mv * ratio / 1000.0f);
    }
}