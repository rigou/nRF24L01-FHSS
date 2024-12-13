/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#include <Arduino.h> // for Serial
#include "Common.h" // for GPIOs definitions
#include "Gpio.h"
#include "User.h"

#if DEBUG_ON == 2
#include "rgSerialBT.h"
#endif

// 0=debug off, 1=output to serial, 2=output to serial and optionally bluetooth with dbtprintln()
#define DEBUG_ON 1
// 0=trace off, 1=output to serial, 2=output to serial and optionally bluetooth with trbtprintln()
#define TRACE_ON 0
#include <rgDebug.h>

extern uint16_t ErrorCounter;  // number of transmission errors per second, updated once/second

// User may add code after this line ------------------------------------------

// Read the potentiometers values using these inputs
const uint8_t DAC_GPIOS[]={USR_CHAN1_GPIO, USR_CHAN2_GPIO, USR_CHAN3_GPIO, USR_CHAN4_GPIO};

// Read the switches values using these inputs
const uint8_t BIN_GPIOS[]={USR_CHAN5_GPIO, USR_CHAN6_GPIO};

// Values read in the ADC inputs will be converted to this range
// (constants copied from ESP32Servo.h)
const uint16_t  MIN_PULSE_WIDTH=500;    // the shortest pulse sent to a servo  
const uint16_t  MAX_PULSE_WIDTH=2500;   // the longest pulse sent to a servo 

// User setup code
void UserSetup(int device_id) {
    trprintf("*** %s %s() begin\n", __FILE_NAME__, __FUNCTION__);

    // set the ADC resolution to 10 bits (0-1023) for probing channels 1,2,3 (potentiometers)
	analogReadResolution(10);

    // initialize the digital inputs (switches)
    for (uint8_t idx=0; idx<sizeof(BIN_GPIOS); idx++)
        pinMode(BIN_GPIOS[idx], INPUT_PULLUP);

#if DEBUG_ON == 2
    char bt_name[7]; // the name listed in the list of available bluetooth devices
	snprintf(bt_name, sizeof(bt_name), "Tx%04X", device_id);
    if (!bt_Begin(bt_name))
		dbprintln("Bluetooth failed");
#endif

	trprintf("*** %s %s() returns\n", __FILE_NAME__, __FUNCTION__);
}

// User loop code : called at the beginning of loop() for each datagram
// Return value: user defined
int UserLoopBegin(void) {
	return 0;
}

/* User loop code : outgoing Msg_Datagram
   set your outgoing message here
   it contains the data that is going to be transmitted to the receiver
   typically this data is a list of potentiometers/switches values or sensor readings
   Update Transceiver.MSGVALUES to match your data length
   Example message layout:
   [0]  value of USR_CHAN1_GPIO (P1)
   [1]  value of USR_CHAN2_GPIO (P2)
   [2]  value of USR_CHAN3_GPIO (P3)
   [3]  value of USR_CHAN4_GPIO (P4)
   [4]  state of USR_CHAN5_GPIO (SW1)
   [5]  state of USR_CHAN6_GPIO (SW2)
   // Return value: user defined
*/
int UserLoopMsg(uint16_t *message) {
    //static uint16_t Debug_print_counter=0; Debug_print_counter++;

    // Read the potentiometers
    for (uint8_t idx=0; idx<sizeof(DAC_GPIOS); idx++) {
        uint16_t value=analogRead(DAC_GPIOS[idx]);
        uint16_t pulse=map(value, 0, 1023, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
        message[idx]=pulse;
        //if (Debug_print_counter%20==0) dbprintf("Chan%d=%d ", idx+1, pulse);
    }
    // Read the switches
    for (uint8_t idx=0; idx<sizeof(BIN_GPIOS); idx++) {
        uint8_t value=digitalRead(BIN_GPIOS[idx]);
        message[idx+sizeof(DAC_GPIOS)]=value;
        //if (Debug_print_counter%20==0) dbprintf("Chan%d=%d ", idx+sizeof(DAC_GPIOS)+1, value);
    }
    //if (Debug_print_counter%20==0) dbprintln("");
    return 0;
}

/* User loop code : incoming Ack_Datagram
   process your ACK datagram here
   the ACK datagram contains the data received from the receiver in response to the previous Msg_Datagram
   typically this data is a list of values related to the receiver state (reception error rate, sensor readings, battery charge)
   Example message layout:
   [0]  Receiver errors count
   [1]  Receiver power supply voltage
   // Return value: user defined
*/
int UserLoopAck(uint16_t *message) {
    // Example: display the Tx error count and the data received from Rx: Rx error count and power supply voltage
    static unsigned long Last_time=0; // ms
    if (millis()>=Last_time+1000) {
#if DEBUG_ON
        uint16_t rx_errors=message[0];  // Rx error count
        uint16_t rx_voltage=message[1]; // Rx power supply voltage
        dbtprintf("Tx %u errors, Rx %u errors, %u mV\n", ErrorCounter, rx_errors, rx_voltage);
#endif
        Last_time=millis();
    }
    return 0;
}
