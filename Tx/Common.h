/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#pragma once
#include <Arduino.h>

// Common library -----------------------------------------

void BlinkLed(byte led_gpio, unsigned int period, unsigned int time_on, bool restart);
void FlashLed(byte led_gpio, unsigned int time_on=0);
void EndProgram(bool reset);
uint32_t GetRandomInt32(void);
uint16_t GetRandomInt16(void);
