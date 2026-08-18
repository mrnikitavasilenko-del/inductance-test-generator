/*
 * 7SegmentC.c
 *
 *  Created on: Feb 19, 2026
 *      Author: Nikita Vasilenko
 */
#include "7SegmentH.h"
#include "stdio.h"
#include <stdint.h>
#include "stm32f1xx.h"
#define Delay 1
uint8_t Digits[11] =
{
	0x3F,//0
	0x6,//1
	0x5B,//2
	0x4F,//3
	0x66,//4
	0x6D,//5
	0x7D,//6
	0x7,//7
	0x7F,//8
	0x6F,//9
	0x0//' '
};
uint8_t letter[8] =
{
	0x39,//C
	0x77,//A
	0x54,//n
	0x79,//E
	0x50,//r
	0x38,//L
	0x3E,//U
	0x40 //'-' (только сегмент g)
};
uint8_t DisplayOff_Flag = 0;
void DisplayPrint3Digits(uint16_t value, uint8_t line)
{
	uint8_t hundreds = value / 100; //сотни
	uint8_t tens = (value / 10) % 10;  // десятки
	uint8_t units = value % 10;   // единицы
	if (line == 1 && DisplayOff_Flag == 0)
	{
		//if (value<100) hundreds = 11;
		//if (value<10) tens = 11;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET);//DIG_1
		GPIOD->BSRR = ( (Digits[hundreds] << 8) | ((~Digits[hundreds] & 0x7F) << 24) );
		HAL_Delay(Delay);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);//DIG_1
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, SET);//DIG_2
		GPIOD->BSRR = ( (Digits[tens] << 8) | ((~Digits[tens] & 0x7F) << 24) );
		HAL_Delay(Delay);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, RESET);//DIG_2
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);//DIG_3
		GPIOD->BSRR = ( (Digits[units] << 8) | ((~Digits[units] & 0x7F) << 24) );
		HAL_Delay(Delay);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);//DIG_3
	}
	if (line == 2 && DisplayOff_Flag == 0)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, SET);//DIG_4
		GPIOD->BSRR = ( (Digits[hundreds] << 8) | ((~Digits[hundreds] & 0x7F) << 24) );
		HAL_Delay(Delay);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, RESET);//DIG_4
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);//DIG_5
		GPIOD->BSRR = ( (Digits[tens] << 8) | ((~Digits[tens] & 0x7F) << 24) );
		HAL_Delay(Delay);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);//DIG_5
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, SET);//DIG_6
		GPIOD->BSRR = ( (Digits[units] << 8) | ((~Digits[units] & 0x7F) << 24) );
		HAL_Delay(Delay);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, RESET);//DIG_6
	}

}
void DisplayPrintU(uint16_t voltage)
{
    // U на первой строке (DIG1)
    GPIOD->BSRR = ((letter[6] << 8) | ((~letter[6] & 0x7F) << 24));
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);

    // Значение на второй строке
    DisplayPrint3Digits(voltage, 2);
}

void DisplayPrintL(uint16_t inductance)
{
    // L
    GPIOD->BSRR = ((letter[5] << 8) | ((~letter[5] & 0x7F) << 24));
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET);   // DIG1
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);

    // -
    GPIOD->BSRR = ((letter[7] << 8) | ((~letter[7] & 0x7F) << 24));
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, SET);   // DIG2
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, RESET);

    // 3 или 6
    if (inductance >= 1000)
        GPIOD->BSRR = ((Digits[3] << 8) | ((~Digits[3] & 0x7F) << 24));
    else
        GPIOD->BSRR = ((Digits[6] << 8) | ((~Digits[6] & 0x7F) << 24));

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);   // DIG3
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);

    // Теперь выводим значение на второй строке
    if (inductance >= 1000)
        DisplayPrint3Digits(inductance / 1000, 2);
    else
        DisplayPrint3Digits(inductance, 2);
}
void DisplayOff(void)
{
	/*if (DisplayOff_Flag == 0) DisplayOff_Flag = 1;
	else {
		DisplayOff_Flag = 0;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);//DIG_1
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, RESET);//DIG_2
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);//DIG_3
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, RESET);//DIG_4
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);//DIG_5
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, RESET);//DIG_6
	}*/
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);//DIG_1
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, RESET);//DIG_2
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);//DIG_3
	HAL_Delay(150);

}
void DisplayCanError(void)
{

	GPIOD->BSRR = ( (letter[0] << 8) | ((~letter[0] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, SET);//DIG_1
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);//DIG_1

	GPIOD->BSRR = ( (letter[1] << 8) | ((~letter[1] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, SET);//DIG_2
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, RESET);//DIG_2

	GPIOD->BSRR = ( (letter[2] << 8) | ((~letter[2] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);//DIG_3
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);//DIG_3


	GPIOD->BSRR = ( (letter[3] << 8) | ((~letter[3] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, SET);//DIG_4
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, RESET);//DIG_4

	GPIOD->BSRR = ( (letter[4] << 8) | ((~letter[4] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);//DIG_5
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);//DIG_5

	GPIOD->BSRR = ( (letter[4] << 8) | ((~letter[4] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, SET);//DIG_6
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, RESET);//DIG_6
	GPIOD->BSRR = (0xFF << 24);
}
void Display_Error(void)
{
	GPIOD->BSRR = ( (letter[3] << 8) | ((~letter[3] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, SET);//DIG_4
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, RESET);//DIG_4

	GPIOD->BSRR = ( (letter[4] << 8) | ((~letter[4] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);//DIG_5
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);//DIG_5

	GPIOD->BSRR = ( (letter[4] << 8) | ((~letter[4] & 0x7F) << 24) );
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, SET);//DIG_6
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, RESET);//DIG_6
	GPIOD->BSRR = (0xFF << 24);
}
