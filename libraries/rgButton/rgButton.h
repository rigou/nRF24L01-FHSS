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

enum BtnStates {BTN_PRESSED, BTN_RELEASED, BTN_REACHED_DURATION};

BtnStates ReadBtn(byte btn_gpio, byte led_gpio, long max_duration);
BtnStates ReadBtnBlock(byte btn_gpio, byte led_gpio, long max_duration);
