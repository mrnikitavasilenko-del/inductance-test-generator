/*
 * 7SegmentH.h
 *
 *  Created on: Feb 19, 2026
 *      Author: Nikita Vasilenko
 */

#ifndef SEVEN_SEGMENTH_H_
#define SEVEN_SEGMENTH_H_
#include <stdint.h>
void DisplayPrint3Digits(uint16_t value, uint8_t line);  // прототип вашей функции
void DisplayOff(void);
void DisplayCanError(void);
void Display_Error(void);
void DisplayPrintU(uint16_t voltage);
void DisplayPrintL(uint16_t inductance);

#endif /* 7SEGMENTH_H_ */
