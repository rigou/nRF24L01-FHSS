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
#include "Common.h" // for GPIOs definitions
#include "Gpio.h"

extern uint16_t ErrorCounter;  // number of transmission errors per second, updated once/second

// Add user code after this line ------------------------------------------

// ADC1 (8 channels, attached to GPIOs 32 – 39
// ADC2 (10 channels, attached to GPIOs 0, 2, 4, 12 - 15 and 25 - 27
// ADC2 is used by the Wi-Fi driver : you can only use ADC2 when the Wi-Fi driver has not been started.
// Read the potentiometers values using these inputs
const byte DAC_GPIOS[]={USR_CHAN1_GPIO, USR_CHAN2_GPIO, USR_CHAN3_GPIO};

// Values read in the ADC inputs will be converted to this range
// (constants copied from ESP32Servo.h)
const uint16_t  MIN_PULSE_WIDTH=500;    // the shortest pulse sent to a servo  
const uint16_t  MAX_PULSE_WIDTH=2500;   // the longest pulse sent to a servo 

// User setup example code
void UserSetup(void) {
    // set the ADC resolution to 10 bits (0-1023) for probing channels 1,2,3 (potentiometers)
	analogReadResolution(10);

    // Channels 4,5 are digital (switches)
    pinMode(USR_CHAN4_GPIO, INPUT_PULLUP);
    pinMode(USR_CHAN5_GPIO, INPUT_PULLUP);
}

/* User loop code : outgoing Msg_Datagram
   set your outgoing message here
   it contains the data that is going to be transmitted to the receiver
   typically this data is a list of potentiometers/switches values or sensor readings
   Make sure your data does not exceed the size defined by Transceiver.MSGVALUES
   Example message layout:
   [0]  state of PAIRING_GPIO (Pairing button)
   [1]  state of USR_CHAN4_GPIO (SW1)
   [2]  state of USR_CHAN5_GPIO (SW2)
   [3]  value of USR_CHAN1_GPIO (P1)
   [4]  value of USR_CHAN2_GPIO (P2)
   [5]  value of USR_CHAN3_GPIO (P3)
*/
void UserLoopMsg(uint16_t *message) {

    // Example code: pressing the pairing button on Tx will light TESTLED on Rx
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
    message[0]=Last_button_state==LOW?1:0;

    // Example code continuation : we use 3 potentiometers and 2 switches to control the remote device
    //
    // Read the switches state (SW1, SW2), no debounce for simplicity
    message[1]=digitalRead(USR_CHAN4_GPIO);
    message[2]=digitalRead(USR_CHAN5_GPIO);
    //
    // Read the potentiometers values (P1, P2, P3)
    for (int idx=0; idx<sizeof(DAC_GPIOS); idx++) {
        uint16_t analogValue = analogRead(DAC_GPIOS[idx]);
        uint16_t pulse=map(analogValue, 0, 1023, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
        message[idx+3]=pulse;
        //Serial.printf("Chan%d:%d=%d ", idx, analogValue, pulse);
    }
    //Serial.println("");
		
}

/* User loop code : incoming Ack_Datagram
   process your ACK datagram here
   the ACK datagram contains the data received from the receiver in response to the previous Msg_Datagram
   typically this data is a list of values related to the receiver state (reception error rate, sensor readings, battery charge)
   Example message layout:
   [0]  Receiver errors count
   [1]  Receiver power supply voltage
*/
void UserLoopAck(uint16_t *message) {

    // Example: display the Tx error count and the data received from Rx
    static unsigned long Last_time=0; // ms
    if (millis()>=Last_time+5000) {
        uint16_t rx_errors=message[0];
        uint16_t rx_voltage=message[1];
        Serial.printf("Tx %u errors, Rx %u errors, %u mV\n", ErrorCounter, rx_errors, rx_voltage);
        Last_time=millis();
    }

    // Example: control TESTLED with the data received in the ACK datagram
    static uint16_t Last_value=255;
    uint16_t value=message[2]; // 0 or 1
    if (value!=Last_value) {  // else do not process identical and consecutive value
        Last_value=value;
        digitalWrite(TESTLED, value?HIGH:LOW);
    }

}
