/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#pragma once

// About GPIO usage:
// 16 GPIOs are available for PWM output: 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
// except those already used here:        12, 13, 14, 15, 16,     18, 19          23

// About ADC usage:
// ADC1 has 8 inputs, attached to GPIOs 32-39. 
//  GPIOs 34-39 are input-only, and they don't have internal pull-up resistors
//  and as such cannot be used to digitalRead the state of a switch
// ADC2 has 10 inputs, attached to GPIOs 0, 2, 4, 12-15 and 25-27
// ADC2 is used by the Wi-Fi driver : you can only use ADC2 when the Wi-Fi driver has not been started.

// GPIOs used on both Tx & Rx -----------------------------

#define RUNLED 2        // 0 if not used
#define ERRLED 14       // 0 if not used
#define TESTLED 15      // 0 if not used
#define PAIRING_GPIO 16 // the Pairing button
// #define SCOPE_PIN 17 // the oscilloscope probe

#define SPI_CE_GPIO 4
#define SPI_CS_GPIO 5
// #define SPI_MOSI_GPIO 23 // default
// #define SPI_MISO_GPIO 19 // default
// #define SPI_SCK_GPIO  18 // default

// Tx GPIOs only ------------------------------------------

// RF output level is hardware-encoded by 2 GPIOs : nc=not connected, gnd=connected to common ground
// 	GPIO	PA_MIN	PA_LOW	PA_HIGH	PA_MAX
// 	 12		  nc	 gnd	  nc	  gnd
// 	 13		  nc	 nc	  	  gnd	  gnd
#define PALEVEL0_GPIO 12
#define PALEVEL1_GPIO 13

// Rx GPIOs only ------------------------------------------

#define POWERSENSOR_GPIO 36 // used only in Rx/User.cpp

// User-defined GPIOs --------------------------------------
//
// see Tx/User.h and Rx/User.h