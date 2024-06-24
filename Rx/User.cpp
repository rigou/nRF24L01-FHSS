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

// Add user code after this line ------------------------------------------

// To use the ESP32Servo library, open the Library Manager in the Arduino IDE and install it from there
#include <ESP32Servo.h>
const byte PWM_GPIOS[]={USR_CHAN1_GPIO, USR_CHAN2_GPIO, USR_CHAN3_GPIO};
Servo Servo_obj[sizeof(PWM_GPIOS)];

// User setup example code
void UserSetup(void) {
     PowerSensor_obj.Config();

    // ESP32Servo setup
    byte idx=0;
	// for (idx=0; idx<4; idx++)
	// 	ESP32PWM::allocateTimer(idx);
	for (idx=0; idx<sizeof(PWM_GPIOS); idx++) {
		Servo_obj[idx].setPeriodHertz(50);
		Servo_obj[idx].attach(PWM_GPIOS[idx], MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
	}

    // Channels 4,5 are digital (controled by switches)
    pinMode(USR_CHAN4_GPIO, OUTPUT);
    pinMode(USR_CHAN5_GPIO, OUTPUT);
}

/* User loop code : incoming Msg_Datagram
   process the MSG datagram here
   it contains the data received from the transmitter
   typically this data is a list of potentiometers/switches values or sensor readings
   Example message layout:
   [0]  state of PAIRING_GPIO (Pairing button)
   [1]  state of USR_CHAN4_GPIO (SW1)
   [2]  state of USR_CHAN5_GPIO (SW2)
   [3]  value of USR_CHAN1_GPIO (P1)
   [4]  value of USR_CHAN2_GPIO (P2)
   [5]  value of USR_CHAN3_GPIO (P3)
*/
void UserLoopMsg(uint16_t *message) {

    // Example: control TESTLED with the data received in the MSG datagram
    digitalWrite(TESTLED, message[0]);

    // Example code continuation : we use 3 servos and 2 digital outputs
    for (int idx=0; idx<sizeof(PWM_GPIOS); idx++) {
        int pulse=message[idx+3];
        Servo_obj[idx].write(pulse);
    }
    digitalWrite(USR_CHAN4_GPIO, message[1]);
    digitalWrite(USR_CHAN5_GPIO, message[2]);
}

/* User loop code : outgoing Ack_Datagram
   set your outgoing message here
   it contains the data that is going to be sent back to the transmitter after receiving the next datagram
   typically this data is a list of values related to the receiver state (reception error rate, sensor readings, battery charge)
   Make sure your data does not exceed the size defined by Transceiver.ACKVALUES
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

    // message[2] = pressing the pairing button on Rx will light TESTLED on Tx
    // notice: the pairing button is not used while MULTIFREQ so user can safely access it
    static byte Last_button_state=HIGH;
    static unsigned long Last_button_time=0; // to debounce button action
    if (millis()>=Last_button_time+500) {
        byte button_state=digitalRead(PAIRING_GPIO);
        if (button_state!=Last_button_state) {
            Last_button_state=button_state;
            Last_button_time=millis();
        }
    }
    message[2]=Last_button_state==LOW?1:0;
}

