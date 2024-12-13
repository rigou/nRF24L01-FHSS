/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

// User *may* modify this file if he knows what he is doing

#pragma once

// These pin assignemnts are optimized for NodeMCU-32 Dev board
// GPIOs available for ADC:     4,     12, 13, 14, 15,                 i2c i2c     25, 26, 27, 32, 33, 34, 35, 36, 39
// GPIOs available for PWM: 0,  4,  5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
// defined in       Gpio.h:     4,  5,             15, 16, 17, 18, 19,         23
// defined in    Tx/User.h: 0,         12, 13, 14,                                 ch7 ch8     ch5 ch6 ch3 ch4 ch1 ch2
// defined in    Rx/User.h:            ch7 ch8 ch6                                 ch3 ch4 ch5 ch1 ch2

// About ADC usage:
// ADC1 has 8 inputs, attached to GPIOs 32-39. 
//  GPIOs 34-39 are input-only, and they don't have internal pull-up resistors
//  and as such cannot be used to digitalRead the state of a switch
// ADC2 has 10 inputs, attached to GPIOs 0, 2, 4, 12-15 and 25-27
//  ADC2 is used by the Wi-Fi driver : you can only use ADC2 when the Wi-Fi driver has not been started.

// GPIOs used by both Tx & Rx -----------------------------

#define RUNLED_GPIO   2 // 0 if not used
#define ERRLED_GPIO  15 // 0 if not used
#define PAIRING_GPIO 16 // the Pairing button

#define SPI_CE_GPIO 17
#define SPI_CS_GPIO  5
// #define SPI_SCK_GPIO  18 // default
// #define SPI_MISO_GPIO 19 // default
// #define SPI_MOSI_GPIO 23 // default

#define POWERSENSOR_GPIO 4 // battery voltage sensor used only in Rx/User.cpp for now, will be implemented in Tx later

// User-defined GPIOs --------------------------------------
//
// see Tx/User.h and Rx/User.h
