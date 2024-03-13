#include <WS2812.h>
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <math.h>
#include <stm32f4xx_hal_spi.h>

// /*
// DK THING
//     // /* Private variables ---------------------------------------------------------*/
//     // SPI_HandleTypeDef hspi1;

//     // UART_HandleTypeDef huart2;

//     // /* USER CODE BEGIN PV */
//     // // Global flags
//     // volatile uint8_t spi_xmit_flag = 0;
//     // volatile uint8_t spi_recv_flag = 0;

//     // /* USER CODE END PV */

//     // /* Private function prototypes -----------------------------------------------*/
//     // void SystemClock_Config(void);
//     // // static void MX_GPIO_Init(void);
//     // // static void MX_USART2_UART_Init(void);
//     // static void MX_SPI1_Init(void);
// */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

// ...

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
//   if (HAL_SPI_Init(&hspi1) != HAL_OK)
//   {
//     Error_Handler();
//   }
  /* USER CODE BEGIN SPI1_Init 2 */
    
  /* USER CODE END SPI1_Init 2 */

}

HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);

// SELF WRITTEN:

int R, G, B, C, X, H_prime;
char flag;
int initFunc(void) // initializes everything we need
{
    BOARD_Init();
    // PWM_Init();
    // TIMER_Init();

// /* DK THING
//     /* MCU Configuration--------------------------------------------------------*/

//     /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//     // HAL_Init();

//     /* Configure the system clock */
//     // SystemClock_Config();

//     /* Initialize all configured peripherals */
//     // MX_GPIO_Init();
//     // MX_USART2_UART_Init();
//     // MX_SPI1_Init();
// */

    // initialize SPI
    MX_SPI1_Init();

    // RGB library
    ws2812_init();
    ws2812_pixel_all(255,255,255);

    return SUCCESS;
}

int main(void)
{
    initFunc();
    while (TRUE)
    {
        HAL_Delay(4000);
        ws2812_send_spi();
    }
}