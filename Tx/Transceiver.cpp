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

#include <bootloader_random.h>
#include "Gpio.h"
#include "Transceiver.h"

// 0=debug off, 1=output to serial, 2=output to serial and optionally bluetooth with dbtprintln()
#define DEBUG_ON 1
// 0=trace off, 1=output to serial, 2=output to serial and optionally bluetooth with trbtprintln()
#define TRACE_ON 0
// the oscilloscope probe ; comment out this line if not used
//#define SCOPE_GPIO 21 
#include "rgDebug.h"

// set CPU FREQ 40-240 MHZ and setRetries(2, 0) for 100 dg/s

Transceiver::Transceiver() {
	dbprintf("using library %s %s\n", RNGLIB_NAME, RNGLIB_VERSION);
	RF24 transceiver(SPI_CE_GPIO, SPI_CS_GPIO, SPI_SPEED);
    Radio_obj=transceiver;
}

// Configure the radio device to transmit with given RF output power
// pa_level values: RF24_PA_MIN (0), RF24_PA_LOW (1), RF24_PA_HIGH (2), RF24_PA_MAX (3) ; definition at line 35 of file RF24.h
// if the 2 devices are separated by a distance < 1 meter then use RF24_PA_MIN for best results
// because using higher pa_level would saturate the receiver and many datagrams would be lost
// Return value: true=OK, false=hardware error (check SPI connections)
bool Transceiver::Setup(bool is_tx, uint16_t tx_device_id, uint16_t rx_device_id, uint16_t mono_channel, uint16_t pa_level) {
	trprintf("*** %s %s() begin\n", __FILE_NAME__, __FUNCTION__);
	trprintf("Mode %s TxId 0x%06x, RxId 0x%06x, Chan 0x%02x (%d), Pa_level %d\n", 
		is_tx?"TX":"RX", tx_device_id, rx_device_id, mono_channel, mono_channel, pa_level);
	bool retval=true; // $$TODO implement error checking
	if (!Radio_obj.begin()) {
#ifdef DEBUG_SERIAL_ENABLED
		dbprintln("radio hardware is not responding");
#endif
		return false;
	}
	setupScope();

	// Set the RF power output and Enable the LNA (Low Noise Amplifier) Gain
	Radio_obj.setPALevel(pa_level, true);
	
	// Enable custom payloads in the acknowledgement datagrams
	// this will automatically enable dynamic payloads on pipe 0 (required for TX mode when expecting ACK payloads) & pipe 1. 
	Radio_obj.enableAckPayload();

	const uint8_t ADDRESS_WIDTH=3; // we want 2-byte addresses only but setAddressWidth() requires min value=3
	Radio_obj.setAddressWidth(ADDRESS_WIDTH);

	// Assign the device ids to the reading/writing pipes
	// we cannot simply pass a pointer to these ids because they are uint16_t and the address width is 3,
	// so the 3rd byte would be undefined (some random value indeed). We must actually pass a pointer to an array of 3 bytes.
	uint8_t tx_id[ADDRESS_WIDTH]; get_bytes(tx_id, tx_device_id, ADDRESS_WIDTH);
	uint8_t rx_id[ADDRESS_WIDTH]; get_bytes(rx_id, rx_device_id, ADDRESS_WIDTH);
	if (is_tx) {
		Radio_obj.openWritingPipe(tx_id);
		Radio_obj.openReadingPipe(1, rx_id);
		dbprintf("Tx WritingPipe address=0x%06x Rx ReadingPipe address=0x%06x\n", tx_device_id, rx_device_id);
	}
	else {
		Radio_obj.openWritingPipe(rx_id);    // device transmits on pipe 0
		Radio_obj.openReadingPipe(1, tx_id); // device receives on pipe 1
		dbprintf("Rx WritingPipe address=0x%06x Rx ReadingPipe address=0x%06x\n", rx_device_id, tx_device_id);
	}

  	// Set CRC size
	Radio_obj.setCRCLength(RF24_CRC_16); // 16 bits is the default but let's be explicit

	// The transmission data rate affects the range and the transmission error rate
	// Higher data rates give shorter range and more errors or more retransmission attempts, and increase the power supply current
	// nRF24L01Plus Supply current at RF24_250KBPS: 12.6 mA
	rf24_datarate_e rates[]={RF24_250KBPS, RF24_1MBPS, RF24_2MBPS};
	Radio_obj.setDataRate(rates[COM_DATARATE]);

	// Set the fixed frequency
    Radio_obj.setChannel(mono_channel);
	MonoChannel=mono_channel; // used later by AssignChannels()

	if (is_tx)
        Radio_obj.stopListening(); // this also discards any unused ACK payloads
	else
		Radio_obj.startListening();  // put device in RX mode

	if (is_tx) {
		// Auto retransmission:
		//  The RF24 chip is capable to retransmit MSG datagrams ART_ATTEMPTS times
		//  if an ACK has not been received after ART_DELAY microseconds
		//
		// ART_DELAY:
		// 	This delay is critical it must be larger than the normal MSG datagram transmission time + ACK datagram receiving time
		// 	ART_DELAY: 0=250µs, 1=500µs, 2=750µs, 3=1000µs, 4=1250µ, 5=1500µs, ... 15=4000µs
		const uint8_t ART_DELAY=COM_ART_DELAY;
		//
		// ART_ATTEMPTS:
		// 	To reduce the transmission error rate, set ART_ATTEMPTS (ART=auto retransmission) between 1 and 15
		// 	 take into account each retransmission will take ART_DELAY µs, reducing the time available for your application data processing
		//	 and obviously ART_DELAY(µs) * ART_ATTEMPTS must be smaller than DGPERIOD
		// 	Alternatively, if transmission errors are acceptable then set ART_ATTEMPTS=0 to disable auto retransmission entirely
		const uint8_t ART_ATTEMPTS=COM_ART_ATTEMPTS;
		Radio_obj.setRetries(ART_DELAY, ART_ATTEMPTS);
	}

	if (is_tx)
		memset(&Msg_Datagram, 0, sizeof(MsgDatagram)); // initialize the first MSG datagram
	else {
		// initialize the calculation of the average delay between received datagrams
		compute_avg_datagram_period(0);

	    // initialize the first ACK datagram for pipe 1
		// The next time a message is received on pipe 1, the data in Ack_Datagram will be sent back in the ACK payload
		memset(&Ack_Datagram, 0, sizeof(AckDatagram));
        Radio_obj.writeAckPayload(1, &Ack_Datagram, sizeof(AckDatagram));
	}

#if DEBUG_ON
	dbprintf("CPU %lu MHz, SPI %u kHz\n", getCpuFrequencyMhz(), SPI_SPEED/1000);
	dbprintln("----------------------------------------");
    Radio_obj.printPrettyDetails(); // debug : print human readable data
	dbprintln("----------------------------------------");
#endif

	trprintf("*** %s %s() returns %d\n", __FILE_NAME__, __FUNCTION__, retval);
	return retval;
}

// Transmit a datagram and acquire the ACK of the previous one from the reception pipe, if any
// MSG/ACKVALUES CPU Freq	  Datarate	Time(µs)
//    14/14		    80			250K	4185
//    14/14		   160			250K	3967
//    14/14		   240			250K	3894
//    10/10		    80			250K	3459
//    10/10		   160			250K	3307
//    10/10		   240			250K	3236
//    10/5		    80			250K	3086	*** recommended values for testing ***
//    10/5		   160			250K	2855
//    10/5		   240			250K	2784
// You can use an oscilloscope to observe these events (macro defined in rgDebug.h) - define SCOPE_GPIO if you need this
// Return value: 
//  true=MSG datagram sent and ACK datagram of previous datagram received
//  false=MSG datagram sent but was not acknowledged with an ACK packet
bool Transceiver::Send(uint16_t msg_type, uint16_t *message) {
	bool retval=true;
	static uint16_t Counter_int=0; // 0 - 65535
	
	writeScope(HIGH);
	//micros_t start_timer = micros();

	Msg_Datagram.number=Counter_int++;
	Msg_Datagram.type=msg_type;
	memcpy(Msg_Datagram.message, message, sizeof(Msg_Datagram.message));

	// write() blocks until the message is successfully acknowledged by the receiver or the timeout/retransmit maxima are reached
	uint8_t pipe; // pipe number that received the ACK datagram
	Radio_obj.write(&Msg_Datagram, sizeof(Msg_Datagram));
	if (Radio_obj.available(&pipe))
		Radio_obj.read(&Ack_Datagram, sizeof(Ack_Datagram));  // read incoming ACK datagram
	else
		retval=false;  // ACK datagram not received

	//dbprintf("Send time=%lu\n", micros() - start_timer);

	writeScope(LOW);
	return retval;
}

// Acquire a message from the reception pipe and send the given ACK datagram
// return immediately if no message available in the reception pipe
// MSG/ACKVALUES CPU Freq	  Datarate	Time(µs)
//    10/5		    80			250K	<1000	*** recommended values for testing ***
// You can use an oscilloscope to observe these events (macro defined in rgDebug.h) - define SCOPE_GPIO if you need this
// Return value: false=received nothing (no message available in the reception pipe), true=received a datagram
bool Transceiver::Receive(uint16_t ack_type, uint16_t *ack_message) {
	//trprintf("*** %s %s() begin\n", __FILE_NAME__, __FUNCTION__);
	bool retval=false;
	if (Radio_obj.available()) {
		writeScope(HIGH);
		Radio_obj.read(&Msg_Datagram, sizeof(Msg_Datagram));  // read incoming message and send outgoing ACK datagram

		// Prepare next outgoing ACK datagram and store it in pipe 1,
		// it will be transmitted by next call to read()
		// and the transmitter will receive it in its pipe 0
		Ack_Datagram.number=Msg_Datagram.number;
		Ack_Datagram.type=ack_type;
		memcpy(Ack_Datagram.message, ack_message, sizeof(Ack_Datagram.message));
		Radio_obj.writeAckPayload(1, &Ack_Datagram, sizeof(Ack_Datagram));

		if (Avg_Datagram_Period==0)
			compute_avg_datagram_period(Msg_Datagram.number);

		retval=true;
		writeScope(LOW);
    }
	//trprintf("*** %s %s() returns %d\n", __FILE_NAME__, __FUNCTION__, retval);
	return retval;
}

// Compute the average delay between AVG_COUNT received datagrams, in microsec
// - Receive() calls this method while Avg_Datagram_Period==0
// - initialize the calculation by calling this method with dg_number=0
// Return value: the average period, or 0 if not yet available
void Transceiver::compute_avg_datagram_period(uint16_t dg_number) {
	static const uint8_t AVG_COUNT=32;
	static micros_t Timer_start=0;
	static uint8_t Count=0;
	static uint16_t Last_dg_number=0;

	// if Rx is started before Tx then the calculation may be erroneous
	// because the timing of the 1st few datagrams sent by Tx seems inaccurate
	// so we ignore them
	static uint8_t Ignored_count=0;

	if (dg_number==0) {
		// initialize the calculation
		Avg_Datagram_Period=0;
		Last_dg_number=dg_number;
		Count=0;
		Timer_start=0;
		Ignored_count=10;
	}
	else {
		if (Ignored_count==0) {
			if (Timer_start==0) {
				// start the timer and do not count the 1st datagram
				Count=0;
				Last_dg_number=dg_number;
				Timer_start=micros();
			}
			else {
				Count++;
				//dbprintf("at %lu count %d dg %d\n", micros(), Count, dg_number);
				if (dg_number==Last_dg_number+1) {
					if (Count==AVG_COUNT)
						Avg_Datagram_Period=(micros()-Timer_start)/AVG_COUNT;
					else
						Last_dg_number=dg_number;
				}
				else {
					// missed a datagram : restart avg computation
					// dbprintf("dg missed after %d/%d\n", Count, AVG_COUNT);
					Timer_start=0;
				}
			}

		}
		else
			Ignored_count--;
	}
}

void Transceiver::PrintMsgDatagram(MsgDatagram datagram) {
	dbprintf("(ch 0x%02x) %04x T%x ", Radio_obj.getChannel(), datagram.number, datagram.type);
	for (uint8_t idx=0; idx<MSGVALUES; idx++)
		dbprintf("0x%04x ", datagram.message[idx]);
}

void Transceiver::PrintAckDatagram(AckDatagram datagram) {
	dbprintf("(ch 0x%02x) %04x T%x ", Radio_obj.getChannel(), datagram.number, datagram.type);
	for (uint8_t idx=0; idx<ACKVALUES; idx++)
		dbprintf("0x%04x ", datagram.message[idx]);
}

// Fill given values_out array with distinct random values in the range 0-max_value except given ignored values
// sizeof(values_out) must be < max_value-1 (eg if max_value=83 then sizeof(values_out)=82)
// key is used to seed the RNG
void Transceiver::arrange_values(
	const unsigned int key, 
	const uint8_t max_value, 
	const uint8_t ignored_value1, 
	const uint8_t ignored_value2,
	const uint8_t sizeof_values_out,
	uint8_t *values_out) {

    const uint8_t NO_VALUE=255;
	const uint8_t all_values=sizeof_values_out+2; // 2 possible values in the range 0-max_value will be ignored
	bool available_values[all_values]; // 2 possible values in the range 0-max_value will be ignored
    int idx=0;
	
	for (idx=0; idx<all_values; idx++)
        available_values[idx]=true;
	// won't be assigned to the values_out[] array : only values_count-2 values are available
	available_values[ignored_value1]=false;
	available_values[ignored_value2]=false;

	for (idx=0; idx<sizeof_values_out; idx++)
        values_out[idx]=NO_VALUE;

	Random_obj.Seed(key);
    for (idx=0; idx<sizeof_values_out; idx++) {
        while (values_out[idx]==NO_VALUE) {
            uint8_t rnd_value=Random_obj.Next(max_value+1); // pseudo-random value in the range (0, max_value)
            if (available_values[rnd_value]) {
                available_values[rnd_value]=false;
				values_out[idx]=rnd_value;
            }
        }
    }
}

// Assign random values to the array of radio channels
// using the key previously set by SetSessionKey()
void Transceiver::AssignChannels(void) {
	//trprintf("AssignChannels(%d)\n", GetSessionKey());
	arrange_values(SessionKey, DEF_MAXCHAN, DEF_MONOCHAN, MonoChannel, sizeof(RF24Channels), RF24Channels);
	/*
	** CAUTION: printing this array takes too long and causes timing issues processing the first datagrams
	for (uint8_t idx=0; idx<sizeof(RF24Channels); idx++) {
		dbprintf("%02d ",RF24Channels[idx]);
		if (idx%10 == 9)
			dbprint('\n');
	}
	dbprint('\n');
	*/
}

// Get the radio channel in use
uint8_t Transceiver::GetChannel(void) {
	return Radio_obj.getChannel();
}

// Use the radio channel corresponding to given datagram number
void Transceiver::SetChannel(uint16_t dg_number) {
	Radio_obj.setChannel(RF24Channels[dg_number % sizeof(RF24Channels)]);
}

void Transceiver::SetPaLevel(int value) {
    Radio_obj.setPALevel(value);
}

uint16_t Transceiver::GetSessionKey(void) {
	return SessionKey;
}
void Transceiver::SetSessionKey(uint16_t key) {
	SessionKey=key;
}

// Extract the 1st "count" bytes (max 8) of "number" into array "bytes"
// if count > sizeof(number) then extra bytes are returned as 0x00
void Transceiver::get_bytes(uint8_t bytes[], uint64_t number, uint8_t count) {
	union {
		uint64_t number;
		uint8_t bytes[sizeof(number)];
	} result;
	result.number=number;
	for (int idx=0; idx<count; idx++)
		bytes[idx]=result.bytes[idx];
}
