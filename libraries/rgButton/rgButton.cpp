/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#include "rgButton.h"

// this is a non-blocking function : call it repeatedly
// if button is released it returns BTN_RELEASED
// if button is pressed :
//	- if max_duration has been reached it returns BTN_REACHED_DURATION once
//	- else it returns BTN_PRESSED
// LED behaviour :
// - LED management is enabled if given led_gpio is non zero
// - it turns on when button is pressed
// - it flashes rapidly when max_duration is reached and button is still pressed 
// - it turns off when button is released
// return value: enum BtnStates {BTN_PRESSED, BTN_RELEASED, BTN_REACHED_DURATION}
BtnStates ReadBtn(uint8_t btn_gpio, uint8_t led_gpio, long max_duration) {
	static bool Last_button_state=BTN_RELEASED; // default
	static unsigned long Button_press_time=0; // ms since BTN_GPIO has been pressed
	static bool Max_duration_complete_bool=false; // prevents returning more than one BTN_REACHED_DURATION while the button has not yet been released
	BtnStates retval=BTN_RELEASED;
	unsigned long now=millis();
	const unsigned int LED_PERIOD=25; // ms

	// when button is pressed then gpio is LOW
	if (digitalRead(btn_gpio)==LOW) { 
		if (Last_button_state==BTN_RELEASED) {
			// button BTN_PRESSED, last state BTN_RELEASED
			// user just pressed the button
			Max_duration_complete_bool=false;
			Button_press_time=now;
			retval=BTN_PRESSED;
		}
		else {
			// button BTN_PRESSED, last state BTN_PRESSED
			// user is still pressing the button
			retval=BTN_PRESSED;
			if (now-Button_press_time >= max_duration && !Max_duration_complete_bool) {
				retval=BTN_REACHED_DURATION;
				Max_duration_complete_bool=true;	
			}
			if (led_gpio) {
				if (Max_duration_complete_bool) {
					// flash Led
					static bool Led_state=LOW;
					static unsigned long Led_flash_time=now; // last time the led changed state (either on or off)
						if (now >= Led_flash_time+LED_PERIOD) {
						Led_state=!Led_state;
						Led_flash_time=now;
						digitalWrite(led_gpio, Led_state);
					}
				}
				else
					digitalWrite(led_gpio, HIGH); // Led stays on until max_duration is reached
			}
		}
		Last_button_state=BTN_PRESSED;
		
	}		
	else {
		// the button is released (gpio is HIGH)
		retval=BTN_RELEASED;
		if (led_gpio && Last_button_state!=BTN_RELEASED)
			digitalWrite(led_gpio, LOW);
		Last_button_state=BTN_RELEASED;
	}
	return retval;
}

// this is the blocking version of function ReadBtn()
// if button is released it returns BTN_RELEASED immediately
// if button is pressed it keeps sampling btn_gpio until button is released :
// - if max_duration has been reached it returns BTN_REACHED_DURATION
// - if button has been released before max_duration it returns BTN_RELEASED
// - LED control is enabled if given led_gpio is non zero
BtnStates ReadBtnBlock(uint8_t btn_gpio, uint8_t led_gpio, long max_duration) {
	BtnStates retval=BTN_PRESSED;
	while (retval==BTN_PRESSED)
		retval=ReadBtn(btn_gpio, led_gpio, max_duration);
	if (retval==BTN_REACHED_DURATION) {
		// flash led until button is released
		BtnStates state=BTN_PRESSED;
		while (state==BTN_PRESSED)
			state=ReadBtn(btn_gpio, led_gpio, max_duration);
	}
	return retval;
}
