/* gpio.c
 *
 * MBI5030 GPIO map:
 *
 *   PC10  --> MBI_DCLK (data clock)
 *   PC11  --> MBI_LE   (latch enable)
 *   PC12  --> MBI_GCLK (PWM via TIM15_CH1, AF, configured in tim.c)
 *   PD0   --> MBI_SDI  (serial data in)
 */

#include "gpio.h"
#include "mbi_prog.h"

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clocks */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* Set outputs LOW before enabling */
    HAL_GPIO_WritePin(MBI_DCLK_GPIO_Port, MBI_DCLK_Pin, GPIO_PIN_RESET);  /* PC10 DCLK */
    HAL_GPIO_WritePin(MBI_LE_GPIO_Port,   MBI_LE_Pin,   GPIO_PIN_RESET);  /* PC11 LE   */
    HAL_GPIO_WritePin(MBI_SDI_GPIO_Port,  MBI_SDI_Pin,  GPIO_PIN_RESET);  /* PD0  SDI  */

    /* PC10 = MBI_DCLK, PC11 = MBI_LE  (PC12 is AF for TIM15, done in tim.c) */
    GPIO_InitStruct.Pin   = MBI_DCLK_Pin | MBI_LE_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PD0 = MBI_SDI */
    GPIO_InitStruct.Pin   = MBI_SDI_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(MBI_SDI_GPIO_Port, &GPIO_InitStruct);
}
