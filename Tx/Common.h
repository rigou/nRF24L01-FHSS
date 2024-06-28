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

// Global settings ----------------------------------------

// About CPU Frequency:
// All our example code is tested with a 80 Mhz CPU frequency
// higher frequency allows you to do more processing in User.cpp, and increase the power supply current
// lower frequency requires you to set COM_TRANS_DGS under 100 datagram per second

// A. User *should* adjust the next 2 values to match what he is doing in User.cpp:

// size of the user data (uint16_t) transmitted  by Tx to Rx, min=4, max=14
// more data takes longer to transmit, implying less time available for your processing in User.cpp
#define COM_MSGVALUES   6   // 6 because our example code demonstrates a 6-channels RC transmitter

// size of the user data (uint16_t) transmitted  by Rx to Tx, min=1, max=14 
// more data takes longer to transmit, implying less time available for your processing in User.cpp
#define COM_ACKVALUES   2   // 2 because our example code puts an error counter and a voltage sensor value in the ACK datagrams

// B. User *may* modify the following values if he knows what he is doing :

// number of datagram transmitted by Tx to Rx per second, must be multiple of 10. Default is 100.
// more datagrams transmitted per second implies less time available for your processing in User.cpp
#define COM_TRANS_DGS   100

// highest radio channel value allowed in your country
// your local laws may not allow the full frequency range (authorized frequencies in France: 2400-2483.5 MHz)
// values: 0-125, actual frequency=2400+channel, eg 2483 MHz for channel 83
#define COM_MAXCHAN     83

// The transmission data rate affects the range and the transmission error rate
// Higher data rates give shorter range and more errors or more retransmission attempts, and increase the power supply current
// values: 0=250 KBPS, 1=1 MBPS, 2=2 MBPS
#define COM_DATARATE    0

// About Auto retransmission:
//  The RF24 chip is capable to retransmit datagrams ART_ATTEMPTS times
//  if an ACK has not been received after ART_DELAY microseconds
//
// ART_DELAY:
// 	This delay is critical it must be larger than the normal MSG datagram (from Tx to Rx) transmission time + ACK datagram (from Rx to Tx) receiving time
// 	ART_DELAY: 0=250µs, 1=500µs, 2=750µs, 3=1000µs, 4=1250µ, 5=1500µs, ... 15=4000µs
// 	ART_DELAY=3 is ok for COM_TRANS_DGS=100,  MSGVALUES=10 ACKVALUES=5 CPUFREQ=80 DATARATE=250KBPS
#define COM_ART_DELAY   3
//
// ART_ATTEMPTS:
// 	To reduce the transmission error rate, set ART_ATTEMPTS between 1 and 15
// 	 take into account each retransmission will take ART_DELAY µs, reducing the time available for your processing in User.cpp
//	 and obviously ART_DELAY(µs) * ART_ATTEMPTS must be smaller than 1000000/COM_TRANS_DGS
// 	Alternatively, if transmission errors are acceptable then set ART_ATTEMPTS=0 to disable auto retransmission entirely
#define COM_ART_ATTEMPTS 0

// Common library -----------------------------------------

void BlinkLed(uint8_t led_gpio, unsigned int period, unsigned int time_on, bool restart);
void FlashLed(uint8_t led_gpio, unsigned int time_on=0);
void EndProgram(bool reset);
uint32_t GetRandomInt32(void);
uint16_t GetRandomInt16(void);
