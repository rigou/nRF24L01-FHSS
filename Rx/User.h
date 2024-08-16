/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#pragma once
#include <cstdint> // for uint16_t

void UserSetup(void);
void UserLoopMsg(uint16_t *message);
void UserLoopAck(uint16_t *message);

// User may add code after this line ------------------------------------------

// Example code: we use 4 servos and 2 Leds, controled by 4 potentiometers and 2 switches on the transmitter
// we have set COM_MSGVALUES and COM_ACKVALUES accordingly in the "Global settings" section of Common.h

#define USR_CHAN1_GPIO 32 // connected to Servo #1
#define USR_CHAN2_GPIO 33 // connected to Servo #2
#define USR_CHAN3_GPIO 25 // connected to Servo #3
#define USR_CHAN4_GPIO 26 // connected to Servo #4
#define USR_CHAN5_GPIO 27 // connected to Led1
#define USR_CHAN6_GPIO 14 // connected to Led2
