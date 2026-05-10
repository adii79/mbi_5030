#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "mbi_prog.h"
#include <string.h>

float power_run_time = 2.0f;

void SystemClock_Config(void);
static void MPU_Config(void);

int main(void)
{
    MPU_Config();
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM15_Init();

    mbi_init();
    mbi_reset_leds();   /* clear all to black */


    PixelColor_t color = {
//         ®  /* FIX: was 1 (nearly zero after scale)  */
     };

     memset(pwm_data, 0, NUMBER_OF_DATA_BYTES * sizeof(uint16_t));
     mbi_set_led(24, color);
     mbi_update();

    /* 6 colors: R G B Y C M */
//    PixelColor_t colors[6] = {
////        /* R1     R2     G1     B1     A1  A2  L1  L2 */
//        {0xFFFF, 0xFFFF, 0,      0,      0, 0, 0, 0}, /* RED     */
//        {0,      0,      0xFFFF, 0,      0, 0, 0, 0}, /* GREEN   */
//        {0,      0,      0,      0xFFFF, 0, 0, 0, 0}, /* BLUE    */
//        {0xFFFF, 0xFFFF, 0xFFFF, 0,      0, 0, 0, 0}, /* YELLOW  */
//        {0,      0,      0xFFFF, 0xFFFF, 0, 0, 0, 0}, /* CYAN    */
//        {0xFFFF, 0xFFFF, 0,      0xFFFF, 0, 0, 0, 0}, /* MAGENTA */
//    };
//
//    PixelColor_t color = {
//    		.A1 = 0,
//			.A2 = 0,
//			.G1 = 0,
//			.L1 = 1,
//			.L2 = 1,
//			.R1 = 0xFFFF,
//			.R2 = 0xFFFF,
//
//    };
//    memset(pwm_data, 0, NUMBER_OF_DATA_BYTES * sizeof(uint16_t));
//    mbi_set_led(1, color);
//    mbi_update();


    int idx = 0;

    while (1)
    {
//        /* Clear buffer — wipe previous color from pwm_data */
//        memset(pwm_data, 0, NUMBER_OF_DATA_BYTES * sizeof(uint16_t));
//
//        /* Set all 24 pixels to current color */
//        for (int p = 1; p <= MAX_LED_CHANNEL; p++)
//            mbi_set_led(p, colors[idx]);
//
//        mbi_update();
//
//        HAL_Delay(1000);
//
//        idx = (idx + 1) % 6;
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 2;
    RCC_OscInitStruct.PLL.PLLN       = 21;
    RCC_OscInitStruct.PLL.PLLP       = 1;
    RCC_OscInitStruct.PLL.PLLQ       = 2;
    RCC_OscInitStruct.PLL.PLLR       = 2;
    RCC_OscInitStruct.PLL.PLLRGE     = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL  = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN   = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                     | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) Error_Handler();
}

void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    HAL_MPU_Disable();
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress      = 0x0;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
