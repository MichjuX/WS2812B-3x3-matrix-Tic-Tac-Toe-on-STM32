/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        uint8_t ch2 = '\r';
        HAL_UART_Transmit(&huart2, &ch2, 1, HAL_MAX_DELAY);
    }

    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return 1;
}

#define BIT_0_TIME		30
#define BIT_1_TIME		60

#define RESET_LEN		50
#define LED_N			60

static uint8_t led_buffer[RESET_LEN + 24 * LED_N + 1];

static void set_byte(uint32_t pos, uint8_t value)
{
  int i;
  for (i = 0; i < 8; i++) {
    if (value & 0x80) {
      led_buffer[pos + i] = BIT_1_TIME;
    } else {
      led_buffer[pos + i] = BIT_0_TIME;
    }

    value <<= 1;
  }
}

void ws2812b_init(void)
{
  int i;
  for (i = 0; i < RESET_LEN; i++)
    led_buffer[i] = 0;

  for (i = 0; i < 24 * LED_N; i++)
    led_buffer[RESET_LEN + i] = BIT_0_TIME;

  led_buffer[RESET_LEN + 24 * LED_N] = 100;

  HAL_TIM_Base_Start(&htim1);
  ws2812b_update();
}

void ws2812b_update(void)
{
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)led_buffer, sizeof(led_buffer));
}

void ws2812b_wait(void)
{
	while (HAL_TIM_GetChannelState(&htim1, TIM_CHANNEL_1) == HAL_TIM_CHANNEL_STATE_BUSY)
		{}
}

void ws2812b_set_color(uint32_t led, uint8_t red, uint8_t green, uint8_t blue)
{
	if (led < LED_N)
	{
		set_byte(RESET_LEN + 24 * led, green);
		set_byte(RESET_LEN + 24 * led + 8, red);
		set_byte(RESET_LEN + 24 * led + 16, blue);
	}
}


////////////////////////////////////////

#define MAX_LED 60
#define USE_BRIGHTNESS 1


uint8_t LED_Data[MAX_LED][4];
uint8_t LED_Mod[MAX_LED][4]; // for brightness

int datasentflag=0;

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	datasentflag=1;
}


void Set_LED (int LEDnum, int Red, int Green, int Blue){
	LED_Data[LEDnum][0] = LEDnum;
	LED_Data[LEDnum][1] = Green;
	LED_Data[LEDnum][2] = Red;
	LED_Data[LEDnum][3] = Blue;
}

#define PI 3.14159265

void Set_Brightness (int brightness){ // 0-45
#if USE_BRIGHTNESS

	if (brightness > 45) brightness =45;
	for (int i=0; i<MAX_LED; i++){
		LED_Mod[i][0] = LED_Data[i][0];
		for (int j=1; j<4; j++){
			float angle = 90-brightness;
			angle = angle*PI / 180;
			LED_Mod[i][j] = (LED_Data[i][j])/(tan(angle));
		}
	}
#endif
}


uint16_t pwmData[(24*MAX_LED)+50];

void WS2812_Send (void)
{
	Set_Brightness(5);
	uint32_t indx=0;
	uint32_t color;


	for (int i= 0; i<MAX_LED; i++)
	{
#if USE_BRIGHTNESS
		color = ((LED_Mod[i][1]<<16) | (LED_Mod[i][2]<<8) | (LED_Mod[i][3]));
#else
		color = ((LED_Data[i][1]<<16) | (LED_Data[i][2]<<8) | (LED_Data[i][3]));
#endif

		for (int i=23; i>=0; i--)
		{
			if (color&(1<<i))
			{
				pwmData[indx] = 60;  // 2/3 of 90
			}

			else pwmData[indx] = 30;  // 1/3 of 90

			indx++;
		}

	}

	for (int i=0; i<50; i++)
	{
		pwmData[indx] = 0;
		indx++;
	}

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);
	while (!datasentflag){};
	datasentflag = 0;
}
uint16_t effStep = 0;

uint8_t rainbow_effect() {
    // Strip ID: 0 - Effect: Rainbow - LEDS: 60
    // Steps: 60 - Delay: 42
    // Colors: 3 (255.0.0, 0.255.0, 0.0.255)
    // Options: rainbowlen=60, toLeft=true,
//  if(millis() - strip_0.effStart < 42 * (strip_0.effStep)) return 0x00;
  float factor1, factor2;
  uint16_t ind;
  for(uint16_t j=0;j<60;j++) {
    ind = (int16_t)effStep + j * 1;
    switch((int)((ind % 60) / 20)) {
      case 0: factor1 = 1.0 - ((float)(ind % 60 - 0 * 20) / 20);
              factor2 = (float)((int)(ind - 0) % 60) / 20;
              Set_LED(j, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2);
              WS2812_Send();
              break;
      case 1: factor1 = 1.0 - ((float)(ind % 60 - 1 * 20) / 20);
              factor2 = (float)((int)(ind - 20) % 60) / 20;
              Set_LED(j, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2);
              WS2812_Send();
              break;
      case 2: factor1 = 1.0 - ((float)(ind % 60 - 2 * 20) / 20);
              factor2 = (float)((int)(ind - 40) % 60) / 20;
              Set_LED(j, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2);
              WS2812_Send();
              break;
    }
  }
  if(effStep >= 60) {effStep=0; return 0x03; }
  else effStep++;
  return 0x01;
}


//extern void HAL_UART_Receive(void *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
//extern void Set_LED(int index, uint8_t R, uint8_t G, uint8_t B);
//extern void WS2812_Send();

void handleDirection(char direction, int *currentLED) {
    // Gaszenie poprzedniej diody
    if (*currentLED == 3) {
        Set_LED(5, 0, 0, 0);
    } else if (*currentLED == 5) {
        Set_LED(3, 0, 0, 0);
    } else {
        Set_LED(*currentLED, 0, 0, 0);
    }

    // Aktualizacja bieżącej diody w zależności od kierunku
    switch (direction) {
        case 'w':
            *currentLED = (*currentLED - 3 + 9) % 9;  // Dioda wyższa
            break;
        case 'a':
            *currentLED = (*currentLED - 1 + 9) % 9;  // Dioda po lewej
            break;
        case 's':
            *currentLED = (*currentLED + 3) % 9;  // Dioda niżej
            break;
        case 'd':
            *currentLED = (*currentLED + 1) % 9;  // Dioda po prawej
            break;
        default:
            // Nieznany kierunek, pozostaw bieżącą diodę bez zmian
            break;
    }

    // Podświetlenie nowej diody na przykład kolorem białym (255, 255, 255)
    if (*currentLED == 3) {
        Set_LED(5, 15, 15, 15);
    } else if (*currentLED == 5) {
        Set_LED(3, 15, 15, 15);
    } else {
        Set_LED(*currentLED, 15, 15, 15);
    }
//    for (int i = 0; i < 5; i++) {
//            Set_LED(*currentLED, 15, 15, 15);
//            WS2812_Send();
//            HAL_Delay(500);  // Miganie przez 500 ms
//            Set_LED(*currentLED, 0, 0, 0);
//            WS2812_Send();
//            HAL_Delay(500);  // Bez światła przez 500 ms
//        }

    // Wysłanie danych do diod LED
    WS2812_Send();
}
void displayBoard(int board[3][3], int LEDS[]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == 1) {
                Set_LED(LEDS[i * 3 + j], 15, 0, 0); // Red for 'X'
            } else if (board[i][j] == 2) {
                Set_LED(LEDS[i * 3 + j], 0, 15, 0); // Green for 'O'
            } else {
                Set_LED(LEDS[i * 3 + j], 0, 0, 0); // Off for empty
            }
        }
    }
    WS2812_Send();
}


/// od gpt
void handleMovement(char direction, int *currentLED, int LEDS[], int board[]) {
    // Update the current LED based on direction without changing the board
	Set_LED(LEDS[*currentLED], 0, 0, 0);
	displayBoard(board, LEDS);
    switch (direction) {
        case 'w':
            *currentLED = (*currentLED - 3 + 9) % 9;  // Move up
            break;
        case 'a':
            *currentLED = (*currentLED - 1 + 9) % 9;  // Move left
            break;
        case 's':
            *currentLED = (*currentLED + 3) % 9;  // Move down
            break;
        case 'd':
            *currentLED = (*currentLED + 1) % 9;  // Move right
            break;
        default:
            break; // Unknown direction, no change
    }
    Set_LED(LEDS[*currentLED], 0, 0, 15);
    WS2812_Send();
}
bool handleSelection(int *currentLED, int player, int board[3][3], int LEDS[]) {
    int row = *currentLED / 3;
    int col = *currentLED % 3;

    // Place the player's symbol if the cell is empty
    if (board[row][col] == 0) {
        board[row][col] = player;
        displayBoard(board, LEDS);
        return true;// Update the LED display
    }
    else return false;
}

int win(int board[3][3] ){
    if((board[0][0]!=0)&&(board[0][0]==board[0][1])&&(board[0][1]==board[0][2])) return 0;
    if((board[1][0]!=0)&&(board[1][0]==board[1][1])&&(board[1][1]==board[1][2])) return 1;
    if((board[2][0]!=0)&&(board[2][0]==board[2][1])&&(board[2][1]==board[2][2])) return 2;
    if((board[0][0]!=0)&&(board[0][0]==board[1][0])&&(board[1][0]==board[2][0])) return 3;
    if((board[0][1]!=0)&&(board[0][1]==board[1][1])&&(board[1][1]==board[2][1])) return 4;
    if((board[0][2]!=0)&&(board[0][2]==board[1][2])&&(board[1][2]==board[2][2])) return 5;
    if((board[0][0]!=0)&&(board[0][0]==board[1][1])&&(board[1][1]==board[2][2])) return 6;
    if((board[0][2]!=0)&&(board[0][2]==board[1][1])&&(board[1][1]==board[2][0])) return 7;
    return -1;
}

void blink(int line, int player, int LEDS[], bool light) {
    int green = 0;
    int red = 0;

    if (light) {
        if (player == 1) {
            green = 15;
        } else {
            red = 15;
        }
    }

    switch (line) {
        case 0:
            Set_LED(LEDS[0], red, green, 0);
            Set_LED(LEDS[1], red, green, 0);
            Set_LED(LEDS[2], red, green, 0);
            break;
        case 1:
            Set_LED(LEDS[3], red, green, 0);
            Set_LED(LEDS[4], red, green, 0);
            Set_LED(LEDS[5], red, green, 0);
            break;
        case 2:
            Set_LED(LEDS[6], red, green, 0);
            Set_LED(LEDS[7], red, green, 0);
            Set_LED(LEDS[8], red, green, 0);
            break;
        case 3:
            Set_LED(LEDS[0], red, green, 0);
            Set_LED(LEDS[3], red, green, 0);
            Set_LED(LEDS[6], red, green, 0);
            break;
        case 4:
            Set_LED(LEDS[1], red, green, 0);
            Set_LED(LEDS[4], red, green, 0);
            Set_LED(LEDS[7], red, green, 0);
            break;
        case 5:
            Set_LED(LEDS[2], red, green, 0);
            Set_LED(LEDS[5], red, green, 0);
            Set_LED(LEDS[8], red, green, 0);
            break;
        case 6:
            Set_LED(LEDS[0], red, green, 0);
            Set_LED(LEDS[4], red, green, 0);
            Set_LED(LEDS[8], red, green, 0);
            break;
        case 7:
            Set_LED(LEDS[2], red, green, 0);
            Set_LED(LEDS[4], red, green, 0);
            Set_LED(LEDS[6], red, green, 0);
            break;
        default:
            // Handle an invalid line value if needed
            break;
    }

    WS2812_Send();
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

//  Set_LED(numer, R, G, B); // numer leda, kolor RGB
//  Set_Brightness(45);
//  WS2812_Send();
  int currentPlayer = 1;

  int board[3][3] = {
	  {0,0,0},
	  {0,0,0},
	  {0,0,0}
  };
  int LEDS[] = {0, 1, 2, 5, 4, 3, 6, 7, 8, 9}; // przeadresowanie ledów w związku z ich spiralnym ułożeniem
  displayBoard(board, LEDS);


  WS2812_Send();



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int currentLED = 0;
  int counter=0;
  int gameWon=-1;
  bool movePossible = true;

  while (1) {
	  uint8_t value;
      HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, &value, 1, 0);
      if(status == HAL_OK){
    	  gameWon = -1;
          for (int i = 0; i < 3; i++) {
              for (int j = 0; j < 3; j++) {
                  board[i][j] = 0;
              }
          }
      }
      while (gameWon<0) {
          if (currentPlayer == 1) {
              Set_LED(9, 15, 0, 0);
          }

          if (currentPlayer == 2) {
              Set_LED(9, 0, 15, 0);
          }
          HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, &value, 1, 0);
          if (status == HAL_OK) {
              switch (value) {
                  case 'w':
                  case 'a':
                  case 's':
                  case 'd':
                      handleMovement(value, &currentLED, LEDS, board);
                      break;

                  case 'f':
                  case 'F':
                      movePossible = handleSelection(&currentLED, currentPlayer, board, LEDS);
                      displayBoard(board, LEDS);
                      if(movePossible){
                    	  currentPlayer = (currentPlayer == 1) ? 2 : 1; // Switch player after placing a marker
                      }
                      else{
                    	  int green=0, red=0;
                    	  if (currentPlayer == 1) {
                    	     green = 15;
                    	  } else {
                    	     red = 15;
                    	  }
                    	  for(int i=0; i<3; i++){
							  Set_LED(LEDS[currentLED], 15, 15, 0);
							  WS2812_Send();
							  HAL_Delay(100);
							  Set_LED(LEDS[currentLED], red, green, 0);
							  WS2812_Send();
							  HAL_Delay(100);
                    	  }
                      }
                      gameWon = win(board);
                      break;

                  case 'r':
                      for (int i = 0; i < 3; i++) {
                          for (int j = 0; j < 3; j++) {
                              board[i][j] = 0;
                          }
                      }
                      displayBoard(board, LEDS);
                      gameWon = -1;
                      break;

                  default:
                      break; // Unknown input, ignore
              }
          } else {
              // No data received within the timeout period
              // Handle the case accordingly
              if (counter > 10000) {
                  counter = 0;
              } else {
                  counter++;
              }

              if (counter % 10 == 0) {
                  displayBoard(board, LEDS);
              }

              if (counter % 20 == 0) {
                  Set_LED(LEDS[currentLED], 0, 0, 15);
              }

              // printf("%d", counter);
              WS2812_Send();
              HAL_Delay(1);
          }
      }
      if (counter > 10000) {
          counter = 0;
      } else {
          counter++;
      }

      if (counter % 10 == 0) {
          displayBoard(board, LEDS);
          blink(gameWon, currentPlayer, LEDS, false);
      }

      if (counter % 20 == 0) {
    	  blink(gameWon, currentPlayer, LEDS, true);
      }

      // printf("%d", counter);
      WS2812_Send();
      HAL_Delay(1);

  }


//  	  uint8_t value;
//  	  HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, &value, 1, 0);
//  	        if (status == HAL_OK) {
//  	            // Data received successfully
//  	            // Process the received data
//  	        	printf("Pressed!\n");
//  	        	WS2812_Send();
//  	        	HAL_Delay(100);
//  	        	  	          // Sprawdzenie otrzymanego znaku
//  	        	switch (value) {
//  	        		case 'w':
//  	        	  	case 'a':
//  	        	  	case 's':
//  	        	  	case 'd':
//  	        	  	// Obsługa kierunku
//  	        	  		Set_LED(LEDS[currentLED], 0, 0, 0);
//						handleDirection(value, &currentLED);
//						break;
//  	        	  	default:
//  	        	  	    // Nieznany znak, ignoruj
//  	        	  		break;
//  	        	}
//  	        } else {
//  	            // No data received within the timeout period
//  	            // Handle the case accordingly
//  	        	if (counter > 10000) {
//  	        	    counter = 0;
//  	        	} else {
//  	        	    counter++;
//  	        	}
//
//  	        	if (counter % 10 == 0) {
//  	        	    Set_LED(LEDS[currentLED], 0, 0, 0);
//  	        	}
//
//  	        	if (counter % 20 == 0) {
//  	        	    Set_LED(LEDS[currentLED], 15, 15, 15);
//  	        	}
//
////  	        	printf("%d", counter);
//  	        	WS2812_Send();
//  	        	HAL_Delay(1);
//  	        }




  //	  for (int i=0; i<9; i++){
  //		  Set_LED(i, 50, 50, 0);
  //
  //	  	  }
  //
  //	  WS2812_Send();
  //	  HAL_Delay(500);
  //	  for (int i=0; i<9; i++){
  //		  Set_LED(i, 255, 0, 0);
  //
  //		  WS2812_Send();
  //		  HAL_Delay(500);
  //	  }

      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 90-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
