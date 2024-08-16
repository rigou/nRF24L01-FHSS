/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#include <Arduino.h> // for Serial
#include "User.h"
#include "PowerSensor.h"
#include "Common.h"
#include "Gpio.h"

extern uint16_t ErrorCounter;  // number of missing datagrams per second, updated once/second

// ADC input at POWERSENSOR_GPIO, compute 2 average values/s, use 10 bit sample resolution
PowerSensor PowerSensor_obj(POWERSENSOR_GPIO, 2, 10);
// 4614 mV sampled as 1728 mV on my system, adjust it for your own hardware
const float POWERSENSOR_CALIBRATION = 4614.0 / 1728.0;

// User may add code after this line ------------------------------------------

// To use the ESP32Servo library, open the Library Manager in the Arduino IDE and install it from there
#include <ESP32Servo.h>

// Move the servos using these PWM outputs
const uint8_t PWM_GPIOS[]={USR_CHAN1_GPIO, USR_CHAN2_GPIO, USR_CHAN3_GPIO, USR_CHAN4_GPIO};
Servo Servo_obj[sizeof(PWM_GPIOS)];

// Turn on/off the Leds using these digital outputs
const uint8_t BIN_GPIOS[]={USR_CHAN5_GPIO, USR_CHAN6_GPIO};

// User setup example code
void UserSetup(void) {
     PowerSensor_obj.Config();

    // ESP32Servo setup
	for (uint8_t idx=0; idx<sizeof(PWM_GPIOS); idx++) {
		Servo_obj[idx].setPeriodHertz(50);
		Servo_obj[idx].attach(PWM_GPIOS[idx], MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
	}

    // Init the digital outputs (Leds)
    for (uint8_t idx=0; idx<sizeof(BIN_GPIOS); idx++)
        pinMode(BIN_GPIOS[idx], OUTPUT);
}

/* User loop code : incoming Msg_Datagram
   process the MSG datagram here
   it contains the data received from the transmitter
   typically this data is a list of potentiometers/switches values or sensor readings
   Example message layout:
   [0]  value of USR_CHAN1_GPIO (P1)
   [1]  value of USR_CHAN2_GPIO (P2)
   [2]  value of USR_CHAN3_GPIO (P3)
   [3]  value of USR_CHAN4_GPIO (P4)
   [4]  state of USR_CHAN5_GPIO (SW1)
   [5]  state of USR_CHAN6_GPIO (SW2)
*/
void UserLoopMsg(uint16_t *message) {
    //static uint16_t Debug_print_counter=0; Debug_print_counter++;

    // Move the servos
    for (uint8_t idx=0; idx<sizeof(PWM_GPIOS); idx++) {
        uint16_t pulse=message[idx];
        Servo_obj[idx].write(pulse);
        //if (Debug_print_counter%20==0) dbprintf("Chan%d=%d ", idx+1, pulse);
    }
    // Turn on/off the Leds
    for (uint8_t idx=0; idx<sizeof(BIN_GPIOS); idx++) {
        uint8_t value=message[idx+sizeof(PWM_GPIOS)];
        digitalWrite(BIN_GPIOS[idx], value);
        //if (Debug_print_counter%20==0) dbprintf("Chan%d=%d ", idx+sizeof(PWM_GPIOS)+1, value);
    }
    //if (Debug_print_counter%20==0) dbprintln("");
}

/* User loop code : outgoing Ack_Datagram
   set your outgoing message here
   it contains the data that is going to be sent back to the transmitter after receiving the next datagram
   typically this data is a list of values related to the receiver state (reception error rate, sensor readings, battery charge)
   Update Transceiver.ACKVALUES to match your data length
   Example message layout:
   [0]  Receiver errors count
   [1]  Receiver power supply voltage
*/
void UserLoopAck(uint16_t *message) {
    
    // Example code:

    // message[0] = reception error rate (number of missing datagrams / second)
    message[0]=ErrorCounter; 

    // message[1] = power supply voltage in millivolts
    static uint16_t Last_voltage=0; // use this buffered value if ReadVoltage() returns false
    uint16_t avg_millivolts;
    if (PowerSensor_obj.ReadVoltage(POWERSENSOR_CALIBRATION, &avg_millivolts))
        Last_voltage=avg_millivolts;
    message[1]=Last_voltage;
}

