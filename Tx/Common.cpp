/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

/******************************************************************************
* WARNING: This file is part of the nRF24L01-FHSS project base code
* and user should not modify it. Any user code stored in this file could be
* made inoperable by subsequent releases of the project.
******************************************************************************/

#include "Common.h"
#include "Gpio.h"
#include <bootloader_random.h> // for GetRandomInt32()

// 0=debug off, 1=output to serial, 2=output to serial and optionally bluetooth with dbtprintln()
#define DEBUG_ON 1
// 0=trace off, 1=output to serial, 2=output to serial and optionally bluetooth with trbtprintln()
#define TRACE_ON 0
#include <rgDebug.h>

// Blink the led, non blocking call : call it repeatedly
// this function can deal only with a single led
// period : time the led is on + time the led is off, in ms
// time_on : time the led is on, in ms
// restart : true=start a new period and turn on the led immediately else simply refresh its state
void BlinkLed(uint8_t led_gpio, unsigned int period, unsigned int time_on, bool restart) {
	//dbprintf("BlinkLed(%d, %d, %d, %d)\n", led_gpio, period, time_on, restart);
	if (led_gpio) {
		static unsigned long Begin_time=0;
		static bool led_state=LOW;

		unsigned long time_now=millis();
		time_on=min(time_on, period);

		if (restart)
			Begin_time=0;

		if (time_now > Begin_time+period && led_state==LOW) {
			Begin_time=time_now; // start a new period
			digitalWrite(led_gpio, HIGH);
			led_state=HIGH;
			//dbprintf("BlinkLed %d: now %lu, begin %lu, period %d : HIGH\n", led_gpio, time_now, Begin_time, period);
		}
		if (time_now > Begin_time+time_on && led_state==HIGH) {
			digitalWrite(led_gpio, LOW);
			led_state=LOW;
			//dbprintf("BlinkLed %d: now %lu, begin %lu, time_on %d : LOW\n", led_gpio, time_now, Begin_time, time_on);
		}
	}
}

// Flash the led once, non blocking call : call it repeatedly
// this function can deal only with a single led
// time_on : time the led is on, in ms, optional
// if time_on is given then turn on the led immediately else simply refresh its state
void FlashLed(uint8_t led_gpio, unsigned int time_on) {
	if (led_gpio) {
		static unsigned long End_time=0;
		static bool led_state=LOW;

		unsigned long time_now=millis();
		if (time_on && led_state==LOW) {
			End_time=time_now+time_on;
			digitalWrite(led_gpio, HIGH);
			led_state=HIGH;
		}
		else if (time_now > End_time && led_state==HIGH) {
			digitalWrite(led_gpio, LOW);
			led_state=LOW;
		}
	}
}

// reset MCU or hold program in infinite loop after fatal error
void EndProgram(bool reset) {
	if (reset) {
        dbprintln("Reset");
		ESP.restart();
		// this point is never reached
	}
	else {
		const int HALTED_DELAY=10000; // ms
		while (1) {
			dbprintln("Program halted");
			if (ERRLED_GPIO) {
				for (uint8_t idx=0; idx<3; idx++) {
					digitalWrite(ERRLED_GPIO, HIGH);
					delay(2000);
					digitalWrite(ERRLED_GPIO, LOW);
					delay(2000);
				}
			}
			else
				delay(HALTED_DELAY*1000);
		}
	}
}

void EndProgram(bool error_condition, const char *error_message) {
	if (error_condition)
		dbprintln(error_message);
}

// get a secure random number from the ESP32 hardware random number generator
// returns a 32 bits random integer suitable for a secret key in the range 1-4294967295
// requires #include <bootloader_random.h>
uint32_t GetRandomInt32(void) {
	uint32_t retval=0;
	bootloader_random_enable();
	// do not return zero
	while (retval==0)
		retval=esp_random();
	bootloader_random_disable();
	return retval;
}

// get a secure random number from the ESP32 hardware random number generator
// returns a 16 bits random integer suitable for a secret key in the range 1-65535
// requires #include <bootloader_random.h>
uint16_t GetRandomInt16(void) {
	return GetRandomInt32() & 0xffff;
}

