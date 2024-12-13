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

void UserSetup(int device_id);
int UserLoopBegin(void);
int UserLoopMsg(uint16_t *message);
int UserLoopAck(uint16_t *message);

// RF output level is hardware-encoded by 2 GPIOs : nc=not connected, gnd=connected to common ground
// 	 GPIO	PA_MIN	PA_LOW	PA_HIGH	PA_MAX
// PALEVEL0	  nc	 gnd	  nc	  gnd
// PALEVEL1   nc	 nc	  	  gnd	  gnd
#define PALEVEL0_GPIO 12
#define PALEVEL1_GPIO 14

// User may add code after this line ------------------------------------------

// Example code: we use 4 potentiometers and 2 switches to control 4 servos and 2 Leds on the receiver
// we have set COM_MSGVALUES and COM_ACKVALUES accordingly in the "Global settings" section of Common.h

#define USR_CHAN1_GPIO 36 // analog, connected to P1
#define USR_CHAN2_GPIO 39 // analog, connected to P2
#define USR_CHAN3_GPIO 34 // analog, connected to P3
#define USR_CHAN4_GPIO 35 // analog, connected to P4
#define USR_CHAN5_GPIO 32 // digital, connected to SW1
#define USR_CHAN6_GPIO 33 // digital, connected to SW2
#define USR_CHAN7_GPIO 25 // possible additional channel on ADC2 available only if WiFi is not used
#define USR_CHAN8_GPIO 26 // possible additional channel on ADC2 available only if WiFi is not used

