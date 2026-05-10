#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdint.h>
#include "mbi_prog.h"
#include "tim.h"

extern float power_run_time;

#define MAX_PWM_VALUE     65280
#define LED_MAX_PWM_VALUE 4095

mainLed_color_t previousPixelColor;
LED_NUM_TYPE    led_num_type_t;
uint8_t         lightsOff = 0;
uint16_t        pwm_data[NUMBER_OF_DATA_BYTES] = { 0 };

/* -----------------------------------------------------------------------
 * mbi_init
 * Start GCLK PWM and send configuration word to all chips.
 * ----------------------------------------------------------------------- */
void mbi_init(void)
{
    HAL_TIM_PWM_Start(&htim15, TIM_CHANNEL_1);
    memset(pwm_data, 0x00, NUMBER_OF_DATA_BYTES * sizeof(pwm_data[0]));
    write_config(PWM_12BIT, 150, NUMBER_OF_CHIPS);
}

/* -----------------------------------------------------------------------
 * scale
 * Convert a 16-bit user value (0..65280) to a 12-bit MBI value (0..4095)
 * with power throttle applied.
 * ----------------------------------------------------------------------- */
static uint16_t scale(uint16_t v)
{
    if (v > MAX_PWM_VALUE) v = MAX_PWM_VALUE;
    float f = (float)v * LED_MAX_PWM_VALUE / (float)MAX_PWM_VALUE;
    f = f * 2.0f / power_run_time;
    if (f > LED_MAX_PWM_VALUE) f = LED_MAX_PWM_VALUE;
    return (uint16_t)f;
}

/* -----------------------------------------------------------------------
 * mbi_set_led
 * pixelIndex: 1-based, 1..24
 *
 * pwm_data layout (NUMBER_OF_CHIPS=12 words per row, 16 rows total):
 *   Row 0..7  = ODD  pixels  (1,3,5,...,23)
 *   Row 8..15 = EVEN pixels  (2,4,6,...,24)
 *
 * For ODD pixel N (1,3,5,...,23):
 *   chip slot = (N-1)/2   range 0..11
 *
 * For EVEN pixel N (2,4,6,...,24):
 *   chip slot = (N-2)/2   range 0..11
 *   base offset = NUMBER_OF_DATA_BYTES/2
 *
 * Channel order in pwm_data rows:
 *   ODD  rows: G1 B1 A2 A1 R2 R1 L2 L1
 *   EVEN rows: G1 B1 A2 A1 L2 L1 R2 R1
 * ----------------------------------------------------------------------- */
void mbi_set_led(uint8_t pixelIndex, PixelColor_t v)
{
    uint16_t R1 = scale(lightsOff ? 0 : v.R1);
    uint16_t R2 = scale(lightsOff ? 0 : v.R2);
    uint16_t G1 = scale(lightsOff ? 0 : v.G1);
    uint16_t B1 = scale(lightsOff ? 0 : v.B1);
    uint16_t A1 = scale(lightsOff ? 0 : v.A1);
    uint16_t A2 = scale(lightsOff ? 0 : v.A2);
    uint16_t L1 = scale(lightsOff ? 0 : v.L1);
    uint16_t L2 = scale(lightsOff ? 0 : v.L2);

    int n = NUMBER_OF_CHIPS; /* 12 */

    if (pixelIndex % 2 == 1) /* ODD: pixels 1,3,5,...,23 */
    {
        int slot = (pixelIndex - 1) / 2; /* 0..11 */
        pwm_data[n * 0 + slot] = G1;
        pwm_data[n * 1 + slot] = B1;
        pwm_data[n * 2 + slot] = A2;
        pwm_data[n * 3 + slot] = A1;
        pwm_data[n * 4 + slot] = R2;
        pwm_data[n * 5 + slot] = R1;
        pwm_data[n * 6 + slot] = L2;
        pwm_data[n * 7 + slot] = L1;
    }
    else /* EVEN: pixels 2,4,6,...,24 */
    {
        int slot = (pixelIndex - 2) / 2; /* 0..11 */
        int base = NUMBER_OF_DATA_BYTES / 2;
        pwm_data[base + n * 0 + slot] = G1;
        pwm_data[base + n * 1 + slot] = B1;
        pwm_data[base + n * 2 + slot] = A2;
        pwm_data[base + n * 3 + slot] = A1;
        pwm_data[base + n * 4 + slot] = L2;
        pwm_data[base + n * 5 + slot] = L1;
        pwm_data[base + n * 6 + slot] = R2;
        pwm_data[base + n * 7 + slot] = R1;
    }
}

/* -----------------------------------------------------------------------
 * mbi_updatePixelColor
 * Thin wrapper kept for backward compatibility with older callers.
 * ----------------------------------------------------------------------- */
void mbi_updatePixelColor(mainLed_color_t m)
{
    PixelColor_t v = {
        m.mainLed_R1, m.mainLed_R2,
        m.mainLed_G1, m.mainLed_B1,
        m.mainLed_A1, m.mainLed_A2,
        m.mainLed_L1, m.mainLed_L2
    };
    if (!m.intensity) {
        memset(&v, 0, sizeof(v));
    }
    mbi_set_led(m.ledIndex, v);
}

/* -----------------------------------------------------------------------
 * mbi_set_led_effect
 * Convenience wrapper: single R/G/B/A/L value per channel pair.
 * ----------------------------------------------------------------------- */
void mbi_set_led_effect(uint8_t LED_NUM, uint16_t LL, uint16_t AA,
                        uint16_t GG, uint16_t RR, uint16_t BB,
                        uint8_t smoothening)
{
    PixelColor_t c = {RR, RR, GG, BB, AA, AA, LL, LL};
    mbi_set_led(LED_NUM, c);
}

/* -----------------------------------------------------------------------
 * mbi_reset_leds
 * Zero the entire pwm_data buffer and push it to the chips.
 * ----------------------------------------------------------------------- */
void mbi_reset_leds(void)
{
    memset(pwm_data, 0, NUMBER_OF_DATA_BYTES * sizeof(pwm_data[0]));
    mbi_update();
}

/* -----------------------------------------------------------------------
 * mbi_update
 * Shift all 16 rows of pwm_data out to the MBI5030 chain.
 *
 * Rows 0..14 use a data-latch (LE pulse on last bit of each row).
 * Row  15    uses a global-latch (LE held across last 3 bits) to
 *            make all new data live simultaneously.
 *
 * NOTE: The MSB mask MUST be 0x8000 (hex). Using the decimal literal
 *       1000000000000000 is > uint16_t max and always masks to zero,
 *       causing all bits to be sent as LOW — a silent data corruption bug.
 * ----------------------------------------------------------------------- */
void mbi_update(void)
{
    uint8_t  n = NUMBER_OF_CHIPS;
    uint16_t data_word;
    uint16_t data_word_bit;
    uint16_t pwm_data_tmp;
    uint8_t  count = 0;

    /* ---- Rows 0..14: data-latch on last bit of each row ---- */
    for (int i = 0; i < 15; i++)
    {
        /* Send first (n-1) full 16-bit words */
        for (data_word = n * count; data_word < n * count + n - 1; data_word++)
        {
            pwm_data_tmp = pwm_data[data_word];
            for (data_word_bit = 0; data_word_bit < 16; data_word_bit++)
            {
                if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
                mbi_clk_high();
                mbi_clk_low();
                pwm_data_tmp <<= 1;
            }
        }

        /* Send first 15 bits of the last word */
        pwm_data_tmp = pwm_data[n * count + n - 1];
        for (data_word_bit = 0; data_word_bit < 15; data_word_bit++)
        {
            if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
            mbi_clk_high();
            mbi_clk_low();
            pwm_data_tmp <<= 1;
        }

        /* Data-latch: assert LE, send bit 16, de-assert LE */
        mbi_latch_high();
        if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
        mbi_clk_high();
        mbi_clk_low();
        mbi_latch_low();

        count++;
    }

    /* ---- Row 15: global-latch (LE spans last 3 bits) ---- */
    for (data_word = n * count; data_word < n * count + n - 1; data_word++)
    {
        pwm_data_tmp = pwm_data[data_word];
        for (data_word_bit = 0; data_word_bit < 16; data_word_bit++)
        {
            if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
            mbi_clk_high();
            mbi_clk_low();
            pwm_data_tmp <<= 1;
        }
    }

    pwm_data_tmp = pwm_data[n * count + n - 1];

    /* Bits 0..12 of last word (13 bits) */
    for (data_word_bit = 0; data_word_bit < 13; data_word_bit++)
    {
        if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
        mbi_clk_high();
        mbi_clk_low();
        pwm_data_tmp <<= 1;
    }

    /* Bit 13 — before LE goes high */
    if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
    mbi_clk_high();
    mbi_clk_low();
    pwm_data_tmp <<= 1;

    /* Global-latch: assert LE, send bits 14 and 15, de-assert LE */
    mbi_latch_high();

    if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
    mbi_clk_high();
    mbi_clk_low();
    pwm_data_tmp <<= 1;

    if (pwm_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
    mbi_clk_high();
    mbi_clk_low();

    mbi_latch_low();
}

/* -----------------------------------------------------------------------
 * write_config
 * Program the MBI5030 configuration register on all n chips.
 * Called once at startup from mbi_init().
 *
 * Same 0x8000 fix applies here — decimal literal bug was in original.
 * ----------------------------------------------------------------------- */
void write_config(uint16_t config_mask, uint8_t current_gain, uint8_t n)
{
    uint16_t config_data = config_mask | ((uint16_t)current_gain << 2);
    uint16_t config_data_tmp;
    uint8_t  bit;

    /* Send full 16-bit config word to chips 2..n */
    for (int i = 1; i <= n - 1; i++)
    {
        config_data_tmp = config_data;
        for (bit = 0; bit < 16; bit++)
        {
            if (config_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
            mbi_clk_high();
            mbi_clk_low();
            config_data_tmp <<= 1;
        }
    }

    /* Chip 1: first 5 bits before LE goes high */
    config_data_tmp = config_data;
    for (bit = 0; bit <= 4; bit++)
    {
        if (config_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
        mbi_clk_high();
        mbi_clk_low();
        config_data_tmp <<= 1;
    }

    mbi_latch_high();

    /* Bits 5..14 with LE high */
    for (bit = 5; bit <= 14; bit++)
    {
        if (config_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
        mbi_clk_high();
        mbi_clk_low();
        config_data_tmp <<= 1;
    }

    /* Last bit: clock high, then drop LE before clock low */
    if (config_data_tmp & 0x8000) mbi_out_high(); else mbi_out_low();
    mbi_clk_high();
    mbi_latch_low();
    mbi_clk_low();
}
