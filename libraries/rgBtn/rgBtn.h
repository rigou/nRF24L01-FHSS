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

#define BTNLIB_NAME	"rgBtn" // spaces not permitted
#define BTNLIB_VERSION	"v1.0.0"

enum BtnStates {BTN_PRESSED, BTN_RELEASED, BTN_REACHED_DURATION};

BtnStates ReadBtn(uint8_t btn_gpio, uint8_t led_gpio, long max_duration);
BtnStates ReadBtnBlock(uint8_t btn_gpio, uint8_t led_gpio, long max_duration);
