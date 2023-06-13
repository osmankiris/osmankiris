#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "string.h"
#define LOG_INTERVAL_MS 1000 // Loglama aralığı (milisaniye cinsinden)
#define FLICKER_INTERVAL_MS 16 // Flicker ölçüm aralığı (milisaniye cinsinden)
#define NUM_SAMPLES 60 // Örnekleme sayısı (örneğin, 1 saniyede 60 örnek)

char *data = "\n";

uint16_t adc_value;
float Vadc;

uint8_t buffer[64];

uint16_t flicker_samples[NUM_SAMPLES];
uint8_t flicker_buffer[64];



ADC_HandleTypeDef hadc1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);

void Read_ADC(){
	HAL_ADC_Start(&hadc1);
	if(HAL_ADC_PollForConversion(&hadc1,10)== HAL_OK){
		adc_value = HAL_ADC_GetValue(&hadc1);
		Vadc = 3.3 * adc_value/4095;
	}

	HAL_ADC_Stop(&hadc1);
}

void Log_ADC_Data() {
    static uint32_t previous_time = 0;
    uint32_t current_time = HAL_GetTick();
    if (current_time - previous_time >= LOG_INTERVAL_MS) {
        previous_time = current_time;

        Read_ADC();

        // Log verisini oluştur
        snprintf((char*)buffer, sizeof(buffer), "ADC Value: %hu, Voltage: %.2fV\n", adc_value, Vadc);

        // Log verisini USB üzerinden gönder
        CDC_Transmit_FS(buffer, strlen((char*)buffer));
    }
}
void Read_Flicker() {
    static uint8_t sample_index = 0;
    static uint32_t previous_time = 0;
    uint32_t current_time = HAL_GetTick();

    if (current_time - previous_time >= FLICKER_INTERVAL_MS) {
        previous_time = current_time;

        HAL_ADC_Start(&hadc1); // ADC dönüşümünü başlat

        // Fotodiyot veya fotosel değerini oku
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint16_t flicker_value = HAL_ADC_GetValue(&hadc1);
            flicker_samples[sample_index] = flicker_value;
        }

        HAL_ADC_Stop(&hadc1); // ADC dönüşümünü durdur

        sample_index++;
        if (sample_index >= NUM_SAMPLES) {
            // Örnekleme tamamlandı, flicker değerini hesapla ve gönder
            float flicker_average = 0.0;
            for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
                flicker_average += flicker_samples[i];
            }
            flicker_average /= NUM_SAMPLES;

            // Flicker değerini tampona formatla
            snprintf((char*)flicker_buffer, sizeof(flicker_buffer), "Flicker: %.2f\n", flicker_average);
            // Flicker değerini USB üzerinden gönder
            CDC_Transmit_FS(flicker_buffer, strlen((char*)flicker_buffer));

            sample_index = 0; // Bir sonraki örnekleme için örnekleyici indeksini sıfırla
        }
    }
}

int main(void)
{

  HAL_Init();


  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USB_DEVICE_Init();

  while (1)
  {
	  CDC_Transmit_FS((uint8_t *)data, strlen(data));
	  HAL_Delay (500);
	  Read_ADC();
	  Log_ADC_Data();
	  Read_Flicker();
  }

}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
