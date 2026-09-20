/******************************************************************************
 * @file           : main.c
 * @brief          : CG2028 Assignment - ElderCare Wearable Safety Companion
 * @author         : Hou Linxin
 * (c) CG2028 Teaching Team
 ******************************************************************************/

/*--------------------------- Includes ---------------------------------------*/
#include "main.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_accelero.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_gyro.h"

#include "sensor_processing.h"
#include "fall_detector.h"
#include "status_indicator.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/*--------------------------- Configuration ----------------------------------*/
#define SAMPLE_MS   20      /* read sensors 50 times a second */
#define PRINT_MS    500     /* one UART line every half second */

static void SystemClock_Config(void);
static void UART1_Init(void);
static void UART_Send(const char *text);
static void read_sensors(int accel_mg[3], int gyro_dps[3]);

UART_HandleTypeDef huart1;

int main(void)
{
    HAL_Init();
    SystemClock_Config();   /* 80 MHz, the BSP I2C timing assumes this */
    UART1_Init();

    BSP_ACCELERO_Init();
    /* Keep 52 Hz, change full scale from +-2 g to +-4 g so impacts do not clip. */
    SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_HIGH, LSM6DSL_ACC_GYRO_CTRL1_XL, 0x38);
    BSP_GYRO_Init();

    /* Clean 3 modules: EWMA -> old val; State: NORMAL; LED clean */
    sensor_processing_init();
    fall_detector_init();
    status_indicator_init();    /* LED2 + user button */

    motion_features_t features = {0};
    fall_state_t state = STATE_NORMAL;

    unsigned int last_sample_time = HAL_GetTick();
    unsigned int last_print_time = HAL_GetTick();

    /* acc_mg   = acceleration magnitude in milli-g
     * gyro_dps = gyroscope magnitude in degrees per second
     * tilt_deg = tilt angle in degrees */
    UART_Send("state,acc_mg,gyro_dps,tilt_deg,still\r\n");

    while (1)
    {
        unsigned int now = HAL_GetTick();

        /* Every 20 ms: read, filter (assembly EWMA), run the state machine. */
        if (now - last_sample_time >= SAMPLE_MS)
        {
            last_sample_time = now;

            // [3] for X,Y,Z 3 axis
            int accel_mg[3];
            int gyro_dps[3];
            read_sensors(accel_mg, gyro_dps);

            sensor_processing_update(accel_mg, gyro_dps, &features);
            state = fall_detector_update(&features, now);
        }

        /* Button press clears a confirmed fall. */
        if (status_indicator_button_pressed())
        {
            fall_detector_ack();
        }

        /* Slow blink normally, fast blink after a fall. Never blocks. */
        status_indicator_update(state, now);

        /* Every 500 ms: one CSV line, plus a warning if asm and C ever disagreed. */
        if (now - last_print_time >= PRINT_MS)
        {
            last_print_time = now;

            char line[64];
            sensor_processing_print(&features, line, sizeof(line));
            UART_Send(fall_state_name(state));
            UART_Send(",");
            UART_Send(line);

            if (sensor_processing_mismatch_count() != 0)
            {
                snprintf(line, sizeof(line),
                         "WARNING: asm/C EWMA mismatches = %d\r\n",
                         sensor_processing_mismatch_count());
                UART_Send(line);
            }
        }
        /* CSV LINE: NORMAL,1058,0,2,1 */
    }
}

/* Accelerometer comes back in mg already. Gyro comes back as float mdps,
 * so divide by 1000 to get whole degrees per second. */
static void read_sensors(int accel_mg[3], int gyro_dps[3])
{
    int16_t accel_raw[3] = {0, 0, 0};
    float gyro_raw[3] = {0.0f, 0.0f, 0.0f};

    BSP_ACCELERO_AccGetXYZ(accel_raw);
    BSP_GYRO_GetXYZ(gyro_raw);

    for (int axis = 0; axis < 3; axis++)
    {
        accel_mg[axis] = (int)accel_raw[axis];
        gyro_dps[axis] = (int)(gyro_raw[axis] / 1000.0f);
    }
}

/* MSI 4 MHz -> PLL -> 80 MHz system clock (same setup as the ST Wi-Fi example). */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState = RCC_MSI_ON;
    osc.MSIClockRange = RCC_MSIRANGE_6;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 40;
    osc.PLL.PLLR = 2;
    osc.PLL.PLLP = 7; //not required
    osc.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        while (1);
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK)
    {
        while (1);
    }
}

static void UART_Send(const char *text)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), HAL_MAX_DELAY);
}

static void UART1_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        while (1) { }
    }
}

/* Do not modify these lines. They suppress UART-related warnings. */
int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    return len;
}
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
int _fstat(int file, struct stat *st) { (void)file; (void)st; return 0; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _close(int file) { (void)file; return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
