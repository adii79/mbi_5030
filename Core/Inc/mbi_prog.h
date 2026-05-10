#ifndef _MBI5030_CASCADE_2_H_
#define _MBI5030_CASCADE_2_H_

#include <stdlib.h>
#include <stdint.h>
#include "main.h"

/* -----------------------------------------------------------------------
 * Pin mapping  (must match gpio.c and your board)
 *
 *  PC12  TIM15_CH1  --> GCLK  (AF, configured in tim.c)
 *  PC10  GPIO OUT   --> DCLK
 *  PC11  GPIO OUT   --> LE
 *  PD0   GPIO OUT   --> SDI
 * ----------------------------------------------------------------------- */
#define MBI_DCLK_Pin        GPIO_PIN_10
#define MBI_DCLK_GPIO_Port  GPIOC

#define MBI_LE_Pin          GPIO_PIN_11
#define MBI_LE_GPIO_Port    GPIOC

#define MBI_SDI_Pin         GPIO_PIN_0
#define MBI_SDI_GPIO_Port   GPIOD

/* -----------------------------------------------------------------------
 * MBI5030 configuration register bit-fields
 * ----------------------------------------------------------------------- */
#define THERMAL_ERROR           0x4000
#define MAX_LED_CHANNEL         24

#define PWM_16BIT               0x0000
#define PWM_12BIT               0x2000

#define PWM_MODE_NORMAL         0x0000
#define PWM_MODE_SCRAMBLE       0x1000

#define DATA_SYNC_AUTO          0x0000
#define DATA_SYNC_MANUAL        0x0400

#define THERMAL_PROTECTION_OFF  0x0000
#define THERMAL_PROTECTION_ON   0x0002

#define MISSING_GSCLK_DET_ON    0x0000
#define MISSING_GSCLK_DET_OFF   0x0001

/* -----------------------------------------------------------------------
 * Chip / data geometry
 * ----------------------------------------------------------------------- */
#define NUMBER_OF_CHIPS         12
#define NUMBER_OF_DATA_BYTES    (NUMBER_OF_CHIPS * 16)
#define DATA_STEP               NUMBER_OF_CHIPS
#define SMALL_STEP              NUMBER_OF_CHIPS
#define BIG_STEP                (NUMBER_OF_CHIPS * 2)

/* -----------------------------------------------------------------------
 * Types
 * ----------------------------------------------------------------------- */
typedef enum { ODD = 0, EVEN = 1 } LED_NUM_TYPE;

typedef struct __attribute__((__packed__)) {
    uint16_t R1, R2, G1, B1, A1, A2, L1, L2;
} PixelColor_t;

typedef struct __attribute__((__packed__)) {
    uint8_t  ledIndex;
    uint8_t  intensity;
    uint16_t mainLed_R1, mainLed_R2;
    uint16_t mainLed_G1;
    uint16_t mainLed_B1;
    uint16_t mainLed_A1, mainLed_A2;
    uint16_t mainLed_L1, mainLed_L2;
} mainLed_color_t;

/* -----------------------------------------------------------------------
 * Low-level GPIO macros
 * ----------------------------------------------------------------------- */
#define mbi_out_high()   HAL_GPIO_WritePin(MBI_SDI_GPIO_Port,  MBI_SDI_Pin,  GPIO_PIN_SET)
#define mbi_out_low()    HAL_GPIO_WritePin(MBI_SDI_GPIO_Port,  MBI_SDI_Pin,  GPIO_PIN_RESET)
#define mbi_clk_high()   HAL_GPIO_WritePin(MBI_DCLK_GPIO_Port, MBI_DCLK_Pin, GPIO_PIN_SET)
#define mbi_clk_low()    HAL_GPIO_WritePin(MBI_DCLK_GPIO_Port, MBI_DCLK_Pin, GPIO_PIN_RESET)
#define mbi_latch_high() HAL_GPIO_WritePin(MBI_LE_GPIO_Port,   MBI_LE_Pin,   GPIO_PIN_SET)
#define mbi_latch_low()  HAL_GPIO_WritePin(MBI_LE_GPIO_Port,   MBI_LE_Pin,   GPIO_PIN_RESET)

/* -----------------------------------------------------------------------
 * Globals declared in mbi_prog.c
 * ----------------------------------------------------------------------- */
extern uint16_t pwm_data[NUMBER_OF_DATA_BYTES];
extern uint8_t  lightsOff;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
void mbi_init(void);
void mbi_set_led(uint8_t pixelIndex, PixelColor_t main_values);
void mbi_updatePixelColor(mainLed_color_t main_values);
void mbi_reset_leds(void);
void mbi_update(void);
void mbi_set_led_effect(uint8_t LED_NUM, uint16_t LL, uint16_t AA,
                        uint16_t GG, uint16_t RR, uint16_t BB,
                        uint8_t smoothening);
void write_config(uint16_t config_mask, uint8_t current_gain, uint8_t n);

#endif /* _MBI5030_CASCADE_2_H_ */
