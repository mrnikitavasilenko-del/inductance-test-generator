/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "7SegmentH.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define U_target 1254				//целевое напряжение 80В (калибровка АЦП1)
#define U_safe 334				//напряжение 20В (калибровка АЦП1)
#define MAX_PULSE_MS 500			// мс, макс. длительность открытого состояния транзистора в импульсе (защита от зависания)
#define I_STEP 5				// А, шаг регулировки I
#define I_MIN 10				// А, минимальная уставка I
#define I_MAX 300				// А, максимальная уставка I
#define L_max 51000				// мкГн, кратно L_STEP_MILLI (макс. уставка = L_max-L_STEP_MILLI = 50000)
#define L_MIN 10				// мкГн, минимальная уставка L
#define L_MILLI_THRESHOLD 1000			// мкГн (=1 мГн) - порог смены шага регулировки L
#define L_STEP_MICRO 10				// мкГн, шаг регулировки L пока L < L_MILLI_THRESHOLD
#define L_STEP_MILLI 1000			// мкГн, шаг регулировки L начиная с L_MILLI_THRESHOLD (1 мГн - один видимый "тик" на экране)
#define dt 20				// мкс, период тика TIM2 = период выборки АЦП2 (см. MX_TIM2_Init)
#define L_SAT_RATIO 0.97f			// порог начала насыщения: L_eff/L_target
#define BUTTON_REPEAT_MS 150			// мс, мин. интервал между шагами I/L при удержании кнопки
#define LAMP_MIN_ON_MS 400			// мс, мин. время свечения лампы "Импульс" (BA4) - реальный импульс часто короче, чем успевает заметить глаз

#define U_ADC_OFFSET 25				// отсчётов АЦП1 при 0В (калибровка 0-100В/5В, МНК)
#define ADC_TO_MV 65.09f			// мВ на 1 отсчёт АЦП1 сверх офсета (калибровка 0-100В/5В, МНК)
#define ADC1_TO_MV(adc) ( ((float)(adc) > U_ADC_OFFSET) ? (((float)(adc) - U_ADC_OFFSET) * ADC_TO_MV) : 0.0f )

#define ADC_TO_MA 82.61f			// мА на 1 отсчёт АЦП2 (калибровка по 3007.xlsx, 35-78А, МНК через ноль)

// Насыщение (case 4): dI по окну из нескольких сэмплов АЦП2 + гистерезис + целочисленная
// арифметика вместо float - см. CLAUDE.md, раздел про state 4, почему одиночный сэмпл
// давал ложные срабатывания.
// Окно/гистерезис зависят от уставки L (см. Sat_Window_Active/Sat_Hysteresis_Active, выставляются
// при старте импульса в case 3): для L ниже SAT_SMALL_L_THRESHOLD_UH бюджет времени на подтверждение
// насыщения (WIN*HYST*dt) становится сравним с самой длительностью импульса до токового предела
// (t~L*I/U) - там используем более быстрые 8/3; для L покрупнее (испытано на стенде на ДУТ 30мГн)
// запас времени большой, поэтому 16/4 нарочно отодвигает срабатывание, чтобы перегиб успел
// развернуться на экране осциллографа - см. CLAUDE.md, раздел "Trade-off for small L".
#define SAT_WINDOW_SAMPLES_SMALL_L 8		// сэмплов АЦП2 на окно, L < SAT_SMALL_L_THRESHOLD_UH
#define SAT_HYSTERESIS_COUNT_SMALL_L 3		// подряд идущих "насыщенных" окон, L < SAT_SMALL_L_THRESHOLD_UH
#define SAT_WINDOW_SAMPLES_LARGE_L 16		// сэмплов АЦП2 на окно, L >= SAT_SMALL_L_THRESHOLD_UH
#define SAT_HYSTERESIS_COUNT_LARGE_L 4		// подряд идущих "насыщенных" окон, L >= SAT_SMALL_L_THRESHOLD_UH - не опускать ниже 4 при окне 16: WIN=16/HYST=3 на бенч-данных (DATA/Newfile1eee.csv) дал ложное срабатывание на 31.85мс/36А (ещё до реального насыщения, от спада наклона из-за сопротивления контура)
#define SAT_SMALL_L_THRESHOLD_UH 1000		// мкГн (=1 мГн) - порог переключения окна/гистерезиса насыщения (не путать с L_MILLI_THRESHOLD - тот про шаг регулировки/индикацию, здесь совпадение значения не принципиально)
#define ADC_TO_MV_FIXED100 6509		// ADC_TO_MV*100, целое (см. ADC_TO_MV)
#define ADC_TO_MA_FIXED100 8261		// ADC_TO_MA*100, целое (см. ADC_TO_MA)
#define L_SAT_RATIO_FIXED100 97		// L_SAT_RATIO*100, целое (см. L_SAT_RATIO)
#define SAT_CUTOFF_DELAY_MS 6		// мс, задержка от подтверждённого saturated=true до реального размыкания транзистора -
					// чтобы "хвостик" насыщения успел развернуться на экране осциллографа (см. CLAUDE.md,
					// раздел про state 4/насыщение). Не касается overcurrent - тот остаётся мгновенным
					// аппаратным пределом, задержка добавляется только для ветки насыщения.
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint8_t Stop,Pusk,I_Plus,I_Minus,L_Plus,L_Minus,L_Pressed,I_Pressed;//buttons&flags
uint8_t state = 1;
uint16_t I=10;
uint16_t L=100;
uint16_t U_adc=0,U, I_adc=0;//u=adc1
uint8_t Measuring_First_Sample;
uint32_t Time;//время
uint32_t Button_Repeat_Time;
uint32_t Pulse_Time;//момент начала импульса (state 4), для защиты от зависания транзистора
volatile uint32_t Tim2_Tick_Count;//инкрементируется в HAL_TIM_PeriodElapsedCallback (TIM2 update IRQ), реальное число тиков TIM2 (dt=20мкс каждый) - не зависит от скорости главного цикла
uint16_t I_window_start;//I_adc в начале текущего окна усреднения dI (насыщение, case 4)
uint32_t Tick_window_start;//Tim2_Tick_Count в начале текущего окна
uint8_t Samples_In_Window;//сколько сэмплов АЦП2 накоплено в текущем окне
uint8_t Saturation_Hit_Count;//сколько подряд окон подряд показали насыщение (гистерезис)
uint8_t Sat_Window_Active;//SAT_WINDOW_SAMPLES_SMALL_L/LARGE_L - выбирается по L при старте импульса (case 3)
uint8_t Sat_Hysteresis_Active;//SAT_HYSTERESIS_COUNT_SMALL_L/LARGE_L - выбирается по L при старте импульса (case 3)
uint8_t Saturation_Confirmed;//saturated уже был true хотя бы раз в этом импульсе - см. SAT_CUTOFF_DELAY_MS
uint32_t Saturation_Confirmed_Time;//HAL_GetTick() в момент первого saturated=true в этом импульсе
uint32_t Lamp_On_Time;//момент включения лампы "Импульс" (BA4), для LAMP_MIN_ON_MS
uint8_t Lamp_Pending_Off;//реальный импульс уже закончился, лампа гаснет отложенно по таймеру
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc2);
  HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_Base_Start_IT(&htim2);//включает update-прерывание TIM2 (см. Tim2_Tick_Count/HAL_TIM_PeriodElapsedCallback) - не влияет на уже настроенный OC-канал
  HAL_ADC_Start(&hadc1);
  HAL_ADC_Start(&hadc2);
  Time=HAL_GetTick()-5000;
  HAL_GPIO_WritePin(BA1_GPIO_Port, BA1_Pin, RESET);//зарядный контактор
  HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, SET);  //разрядный контактор
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	 if( HAL_ADC_GetValue(&hadc1) < U_safe ) HAL_GPIO_WritePin(BA3_GPIO_Port, BA3_Pin, SET);//безопасное напряжение
	 else HAL_GPIO_WritePin(BA3_GPIO_Port, BA3_Pin, RESET);//безопасное напряжение

	 if (Lamp_Pending_Off && (HAL_GetTick() - Lamp_On_Time) >= LAMP_MIN_ON_MS)
	 {//реальный импульс (транзистор/Sync) уже закончился раньше - лампа "Импульс" гаснет только теперь, чтобы глаз успел увидеть вспышку
		 HAL_GPIO_WritePin(BA4_GPIO_Port, BA4_Pin, RESET);
		 Lamp_Pending_Off = 0;
	 }

	 U_adc = HAL_ADC_GetValue(&hadc1);
	 U = (uint16_t)(ADC1_TO_MV(U_adc) / 1000.0f);//напряжение в вольтах (для индикации)

	 switch (state){
	 	 case 1:									//чтение кнопок, отображение U

	 		if ( (HAL_GetTick()-Time)>5000 ){
	 			 L_Pressed = 0;
	 			 I_Pressed = 0;
				 DisplayPrintU(U);	//отображение напряжения через 5 сек
			 }
			 else {									//отображаем L/I
				 if (I_Pressed)
				 {
					 DisplayPrint3Digits(I, 1);	//отображаем I
				 }
				 if (L_Pressed)
				 {
					 DisplayPrintL(L);	// отображаем L
				 }
			 }

	 		 Stop=HAL_GPIO_ReadPin(BE1_GPIO_Port, BE1_Pin);//стоп
	 		 Pusk=HAL_GPIO_ReadPin(BE2_GPIO_Port, BE2_Pin);//Пуск
	 		 I_Plus=HAL_GPIO_ReadPin(BE3_GPIO_Port, BE3_Pin);//I+
	 		 I_Minus=HAL_GPIO_ReadPin(BE4_GPIO_Port, BE4_Pin);//I-
	 		 L_Plus=HAL_GPIO_ReadPin(BE5_GPIO_Port, BE5_Pin);//L+
	 		 L_Minus=HAL_GPIO_ReadPin(BE6_GPIO_Port, BE6_Pin);//L-

	 		 if ((I_Plus+I_Minus+L_Plus+L_Minus) != 0) {
	 			 state=2;							//нажата кнопка регулировки, отображаем параметры
	 		 }
	 		 if (Pusk && !Stop) {
	 			 HAL_GPIO_WritePin(BA1_GPIO_Port, BA1_Pin, SET);//зарядный контактор
	 			 HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, RESET);  //разрядный контактор
	 			 state=3;						//нажата пуск, переход к шагу 3
	 			 Time=HAL_GetTick();
	 		 }


	 		 break;

	 	 case 2:									//кнопки нажаты, Отображение
	 		 Time = HAL_GetTick();
	 		 if ((HAL_GetTick() - Button_Repeat_Time) >= BUTTON_REPEAT_MS)
	 		 {
	 			 Button_Repeat_Time = HAL_GetTick();
	 			 if ( (I_Plus+I_Minus) !=0)
	 			 {
	 				 if (I_Plus) I+=I_STEP;
	 				 else I-=I_STEP;
	 				 if (I < I_MIN) I=I_MIN;
	 				 if (I > I_MAX) I=I_MAX;
	 				 DisplayPrint3Digits(I, 1);	//отображаем I
	 				 I_Pressed= 1;
	 				 L_Pressed =0;
	 			 }
				 if ( (L_Plus+L_Minus) != 0)
				 {
					 uint16_t L_step = (L < L_MILLI_THRESHOLD) ? L_STEP_MICRO : L_STEP_MILLI;//мкГн пока микро, мГн - шаг 1000
					 if (L_Plus) {
						 L+=L_step;
					 } else if (L == L_MILLI_THRESHOLD) {
						 L = L_MILLI_THRESHOLD - L_STEP_MICRO;
					 } else {
						 L-=L_step;
					 }
					 if (L==0) L=L_MIN;
					 if (L>=L_max) L=L_max-L_STEP_MILLI;
					 DisplayPrintL(L);	// отображаем L
					 L_Pressed = 1;
					 I_Pressed = 0;
				 }
	 		 }
			 state = 1;								//вернулись к считыванию
			 break;

	 	 case 3: 									//нажат пуск. отображение U, заряд, отслеживание времени.

	 		 Stop=HAL_GPIO_ReadPin(BE1_GPIO_Port, BE1_Pin);//стоп
	 		 if (Stop) {								//прервали заряд вручную
	 			 state = 5;			//переходим к разряду
	 			 HAL_GPIO_WritePin(BA1_GPIO_Port, BA1_Pin, RESET);//зарядный контактор
	 			 break;
	 		 }

	 		 DisplayPrintU(U); 		//отображение напряжения
	 		 U_adc=HAL_ADC_GetValue(&hadc1);			//вопрос какой АЦП
	 		 if (U_adc>U_target) {
	 			 state =4;			//зарядились, переходим к 4
	 			 HAL_GPIO_WritePin(BA1_GPIO_Port, BA1_Pin, RESET);//зарядный контактор
	 			 HAL_GPIO_WritePin(UPR_Down_GPIO_Port, UPR_Down_Pin, SET);//замыкание транзистора, старт импульса
	 			 HAL_GPIO_WritePin(BA4_GPIO_Port, BA4_Pin, SET);//лампа "Импульс"
	 			 HAL_GPIO_WritePin(Sync_GPIO_Port, Sync_Pin, SET);//синхросигнал для осциллографа
	 			 Measuring_First_Sample = 1;
	 			 Pulse_Time = HAL_GetTick();
	 			 Lamp_On_Time = Pulse_Time;
	 			 Lamp_Pending_Off = 0;
	 			 if (L < SAT_SMALL_L_THRESHOLD_UH) {
	 				 Sat_Window_Active = SAT_WINDOW_SAMPLES_SMALL_L;
	 				 Sat_Hysteresis_Active = SAT_HYSTERESIS_COUNT_SMALL_L;
	 			 } else {
	 				 Sat_Window_Active = SAT_WINDOW_SAMPLES_LARGE_L;
	 				 Sat_Hysteresis_Active = SAT_HYSTERESIS_COUNT_LARGE_L;
	 			 }
	 			 Saturation_Confirmed = 0;
	 		 }
	 		 if ( (HAL_GetTick()-Time)>105000 ){		//ошибка заряда по времени (105с)
	 			state = 6;
	 			HAL_GPIO_WritePin(BA1_GPIO_Port, BA1_Pin, RESET);//зарядный контактор
	 			HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, SET);  //разрядный контактор
	 		 }
	 		 break;

	 	 case 4://зарядились, импульс тока, контроль I и насыщения
	 		 if ( (HAL_GetTick()-Pulse_Time) > MAX_PULSE_MS ){	//транзистор завис открытым дольше расчётного максимума
	 			 HAL_GPIO_WritePin(UPR_Down_GPIO_Port, UPR_Down_Pin, RESET);//размыкание транзистора
	 			 Lamp_Pending_Off = 1;//лампа "Импульс" гаснет отложенно, см. LAMP_MIN_ON_MS
	 			 HAL_GPIO_WritePin(Sync_GPIO_Port, Sync_Pin, RESET);//синхросигнал для осциллографа
	 			 HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, SET);  //разрядный контактор
	 			 state = 6;//ошибка
	 			 break;
	 		 }
	 		 if (__HAL_ADC_GET_FLAG(&hadc2, ADC_FLAG_EOC))
	 		 {
	 			 I_adc = HAL_ADC_GetValue(&hadc2);//чтение DR аппаратно снимает EOC на F1

	 			 if (Measuring_First_Sample)
	 			 {
	 				 I_window_start = I_adc;
	 				 Tick_window_start = Tim2_Tick_Count;
	 				 Samples_In_Window = 0;
	 				 Saturation_Hit_Count = 0;
	 				 Measuring_First_Sample = 0;
	 				 break;
	 			 }

	 			 uint8_t overcurrent = ((uint32_t)I_adc * ADC_TO_MA_FIXED100) >= ((uint32_t)I * 100000u);//I задаётся в А, переводим в мА (×100 для ADC_TO_MA_FIXED100)
	 			 uint8_t saturated = 0;

	 			 Samples_In_Window++;
	 			 if (Samples_In_Window >= Sat_Window_Active)
	 			 {
	 				 int32_t dI_window = (int32_t)I_adc - (int32_t)I_window_start;
	 				 uint32_t ticks_elapsed = Tim2_Tick_Count - Tick_window_start;//реальное число тиков TIM2 за окно - устойчиво к пропущенным сэмплам АЦП2

	 				 if (dI_window > 0 && ticks_elapsed > 0)
	 				 {
	 					 int64_t numerator = (int64_t)((int32_t)U_adc - U_ADC_OFFSET) * ADC_TO_MV_FIXED100 * ((int64_t)ticks_elapsed * dt);
	 					 int64_t denominator = (int64_t)dI_window * ADC_TO_MA_FIXED100;
	 					 int32_t L_eff_uH = (int32_t)(numerator / denominator);//множители ×100 (ADC_TO_MV_FIXED100/ADC_TO_MA_FIXED100) сокращаются

	 					 if (L_eff_uH < (int32_t)(((int32_t)L * L_SAT_RATIO_FIXED100) / 100)) Saturation_Hit_Count++;//достигли начала насыщения
	 					 else Saturation_Hit_Count = 0;
	 				 }
	 				 else
	 				 {
	 					 Saturation_Hit_Count = 0;//ток за окно не вырос - не насыщение, сброс серии
	 				 }

	 				 I_window_start = I_adc;
	 				 Tick_window_start = Tim2_Tick_Count;
	 				 Samples_In_Window = 0;
	 			 }
	 			 saturated = (Saturation_Hit_Count >= Sat_Hysteresis_Active);

	 			 if (saturated && !Saturation_Confirmed)
	 			 {
	 				 Saturation_Confirmed = 1;//насыщение подтверждено впервые в этом импульсе - запускаем задержку SAT_CUTOFF_DELAY_MS
	 				 Saturation_Confirmed_Time = HAL_GetTick();
	 			 }
	 			 uint8_t saturation_cutoff = Saturation_Confirmed && ((HAL_GetTick() - Saturation_Confirmed_Time) >= SAT_CUTOFF_DELAY_MS);

	 			 if (overcurrent || saturation_cutoff)
	 			 {
	 				 HAL_GPIO_WritePin(UPR_Down_GPIO_Port, UPR_Down_Pin, RESET);//размыкание транзистора
	 				 Lamp_Pending_Off = 1;//лампа "Импульс" гаснет отложенно, см. LAMP_MIN_ON_MS
	 				 HAL_GPIO_WritePin(Sync_GPIO_Port, Sync_Pin, RESET);//синхросигнал для осциллографа
	 				 state = 5;//штатное завершение - переходим к разряду
	 			 }
	 		 }
	 		 break;

	 	 case 5: //разряд, переход к 1
	 		 HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, SET);  //разрядный контактор
	 		 state = 1;
	 		 break;

	 	 case 6://ошибки, сброс, переход к 1
	 		 Display_Error();
	 		 I_Plus=HAL_GPIO_ReadPin(BE3_GPIO_Port, BE3_Pin);//I+
			 I_Minus=HAL_GPIO_ReadPin(BE4_GPIO_Port, BE4_Pin);//I-
			 L_Plus=HAL_GPIO_ReadPin(BE5_GPIO_Port, BE5_Pin);//L+
			 L_Minus=HAL_GPIO_ReadPin(BE6_GPIO_Port, BE6_Pin);//L-
			 HAL_GPIO_WritePin(UPR_Down_GPIO_Port, UPR_Down_Pin, RESET);//размыкание транзистора
			 HAL_GPIO_WritePin(Sync_GPIO_Port, Sync_Pin, RESET);//синхросигнал для осциллографа
			 HAL_GPIO_WritePin(BA1_GPIO_Port, BA1_Pin, RESET);//зарядный контактор
			 HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, SET);  //разрядный контактор

			 if ((I_Plus+I_Minus+L_Plus+L_Minus) == 4)
			 {
				 state=1;										//сброс ошибки
				 HAL_GPIO_WritePin(BA2_GPIO_Port, BA2_Pin, RESET);  //разрядный контактор
			 }
	 		 break;

	 }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL_NONE;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the Systick interrupt time
  */
  __HAL_RCC_PLLI2S_ENABLE();
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

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_CC2;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 719;			// 20 мкс @ 36 МГц (см. dt в USER CODE BEGIN PD)
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;//ACTIVE даёт фронт OC2REF только один раз при первом совпадении и не сбрасывается - АЦП2 не видит повторных фронтов; PWM1 переключает каждый период
  sConfigOC.Pulse = 360;//не 0 - иначе событие сравнения совпадает с обнулением CNT и ADC2 не ретриггерится
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, BA3_Pin|BA4_Pin|UPR_Down_Pin|Sync_Pin
                          |BA1_Pin|BA2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Dig_1_Pin|Dig_2_Pin|Dig_3_Pin|Dig_4_Pin
                          |Dig_5_Pin|Dig_6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, Seg_A_Pin|Seg_B_Pin|Seg_C_Pin|Seg_D_Pin
                          |Seg_E_Pin|Seg_F_Pin|Seg_G_Pin|Seg_DP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BA3_Pin BA4_Pin UPR_Down_Pin Sync_Pin
                           BA1_Pin BA2_Pin */
  GPIO_InitStruct.Pin = BA3_Pin|BA4_Pin|UPR_Down_Pin|Sync_Pin
                          |BA1_Pin|BA2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : BE1_Pin BE2_Pin BE3_Pin BE4_Pin
                           BE5_Pin BE6_Pin */
  GPIO_InitStruct.Pin = BE1_Pin|BE2_Pin|BE3_Pin|BE4_Pin
                          |BE5_Pin|BE6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Dig_1_Pin Dig_2_Pin Dig_3_Pin Dig_4_Pin
                           Dig_5_Pin Dig_6_Pin */
  GPIO_InitStruct.Pin = Dig_1_Pin|Dig_2_Pin|Dig_3_Pin|Dig_4_Pin
                          |Dig_5_Pin|Dig_6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Seg_A_Pin Seg_B_Pin Seg_C_Pin Seg_D_Pin
                           Seg_E_Pin Seg_F_Pin Seg_G_Pin Seg_DP_Pin */
  GPIO_InitStruct.Pin = Seg_A_Pin|Seg_B_Pin|Seg_C_Pin|Seg_D_Pin
                          |Seg_E_Pin|Seg_F_Pin|Seg_G_Pin|Seg_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2)
	{
		Tim2_Tick_Count++;//только счётчик реальных тиков TIM2 (dt=20мкс), никакой логики state-machine здесь нет
	}
}
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
#ifdef USE_FULL_ASSERT
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
