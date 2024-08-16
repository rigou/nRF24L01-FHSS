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

// set CPU FREQ 40-240 MHZ and setRetries(2, 0) for 100 dg/s

Transceiver::Transceiver() {
	RF24 transceiver(SPI_CE_GPIO, SPI_CS_GPIO, SPI_SPEED);
    Radio_obj=transceiver;
}

// Configure the radio device to transmit with given RF output power
// pa_level values: RF24_PA_MIN (0), RF24_PA_LOW (1), RF24_PA_HIGH (2), RF24_PA_MAX (3) ; definition at line 35 of file RF24.h
// if the 2 devices are separated by a distance < 1 meter then use RF24_PA_MIN for best results
// because using higher pa_level would saturate the receiver and many datagrams would be lost
// Return value: true=OK, false=hardware error (check your SPI connections)
bool Transceiver::Config(bool is_tx, uint16_t tx_device_id, uint16_t mono_channel, uint16_t pa_level) {

	if (!Radio_obj.begin()) {
#ifdef DEBUG_SERIAL_ENABLED
		dbprintln("radio hardware is not responding");
#endif
		return false;
	}

	// turn off the radio during configuration
	Radio_obj.powerDown();

	// the RF24 library requires that custom ACK payloads are dynamically sized
	// but we don't make use of this feature because our MsgDatagram and AckDatagram payloads have fixed sizes
	Radio_obj.enableDynamicPayloads();  

	// Enable auto acknowledgement
	Radio_obj.enableAckPayload();
  
  	// Set CRC size
	Radio_obj.setCRCLength(RF24_CRC_16); // 16 bits is the default but let's be explicit

	// The transmission data rate affects the range and the transmission error rate
	// Higher data rates give shorter range and more errors or more retransmission attempts, and increase the power supply current
	// nRF24L01Plus Supply current at RF24_250KBPS: 12.6 mA
	rf24_datarate_e rates[]={RF24_250KBPS, RF24_1MBPS, RF24_2MBPS};
	Radio_obj.setDataRate(rates[COM_DATARATE]);

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
	//
	Radio_obj.setRetries(ART_DELAY, ART_ATTEMPTS);  

	HotConfig(is_tx, tx_device_id, mono_channel, pa_level);

	if (is_tx) {
		// initialize the first MSG datagram
		memset(&Msg_Datagram, 0, sizeof(MsgDatagram));
	}
	else {
	    // initialize the first ACK datagram and put it in the transmitting pipe (pipe 0)
		memset(&Ack_Datagram, 0, sizeof(AckDatagram));
        Radio_obj.writeAckPayload(1, &Ack_Datagram, sizeof(AckDatagram));
	}

	// turn on the radio after configuration
    Radio_obj.powerUp();

	dbprintf("CPU %lu MHz, SPI %u kHz\n", getCpuFrequencyMhz(), SPI_SPEED/1000);
	dbprintln("----------------------------------------");
    Radio_obj.printPrettyDetails(); // debug : print human readable data
	dbprintln("----------------------------------------");
	return true;
}

// Reconfigure the transceiver with given parameters
// allows hot switching between normal mode and pairing mode 
// Does not perform all required initializations though, so you still must call Config() at boot
void Transceiver::HotConfig(bool is_tx, uint16_t tx_device_id, uint16_t mono_channel, uint16_t pa_level) {
	// Set the RF power output and Enable the LNA (Low Noise Amplifier) Gain
	Radio_obj.setPALevel(pa_level, true);
	
	Radio_obj.flush_tx();
	Radio_obj.flush_rx();
	SetTransmissionId(tx_device_id);

	if (is_tx) {
		// reassign given transmission id to the transmitting pipe (pipe 0))
        Radio_obj.stopListening(); // this also discards any unused ACK payloads
		Radio_obj.openWritingPipe(TransmissionId);
	}
	else {
		// reassign given transmission id to the reading pipe (pipe 1)
		Radio_obj.closeReadingPipe(1);
		Radio_obj.openReadingPipe(1, TransmissionId);
		Radio_obj.startListening();  // $BUGBUG required ?
	}

	// Set the fixed frequency
    Radio_obj.setChannel(mono_channel);
	MonoChannel=mono_channel; // used later by AssignChannels()
	dbprintf("TxId %x, Chan %d, Pa_level %d\n", tx_device_id, mono_channel, pa_level);
}

// Execution time
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
//
// Return value: 
//  true=MSG datagram sent and ACK datagram of previous datagram received
//  false=MSG datagram sent but was not acknowledged with an ACK packet
bool Transceiver::Send(uint16_t msg_type, uint16_t *message) {
	bool retval=true;
	static uint16_t Counter_int=0; // 0 - 65535
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
	return retval;
}

// Execution time
// MSG/ACKVALUES CPU Freq	  Datarate	Time(µs)
//    10/5		    80			250K	<1000	*** recommended values for testing ***
//
// Return value: false=received nothing, true=received a datagram
bool Transceiver::Receive(uint16_t ack_type, uint16_t *ack_message) {
	bool retval=false;
	if (Radio_obj.available()) {
		Radio_obj.read(&Msg_Datagram, sizeof(Msg_Datagram));  // read incoming message and send outgoing ACK datagram

		// Prepare next outgoing ACK datagram and store it in pipe 1,
		// it will be transmitted by next call to read()
		// and the transmitter will receive it in its pipe 0
		Ack_Datagram.number=Msg_Datagram.number;
		Ack_Datagram.type=ack_type;
		memcpy(Ack_Datagram.message, ack_message, sizeof(Ack_Datagram.message));
		Radio_obj.writeAckPayload(1, &Ack_Datagram, sizeof(Ack_Datagram));

		if (Avg_Datagram_Period==0)
			compute_avg_datagram_period();
		retval=true;
    }
	return retval;
}

// Compute the average delay between AVG_COUNT received datagrams, in microsec,
//	and store the result in Avg_Datagram_Period.
// - Receive() calls this method while Avg_Datagram_Period==0
// - set Avg_Datagram_Period=0 before calling this method
// - this method ignores the first AVG_COUNT received datagrams
// - Once Avg_Datagram_Period is computed, Rx transitions from SYNCHRONIZING to MONOFREQ
void Transceiver::compute_avg_datagram_period(void) {
	static const uint8_t AVG_COUNT=32;
	static micros_t First_datagram_time=0;
	static uint16_t First_datagram_number=Msg_Datagram.number;
	static uint16_t Previous_datagram_number=Msg_Datagram.number-1;
	if (Msg_Datagram.number == Previous_datagram_number+1) {
		if (First_datagram_time==0 && Msg_Datagram.number >= First_datagram_number+AVG_COUNT)
			First_datagram_time=micros(); // start timing
		if (First_datagram_time && Msg_Datagram.number >= First_datagram_number+2*AVG_COUNT) {
			// stop timing and compute average delay
			Avg_Datagram_Period=(micros()-First_datagram_time)/AVG_COUNT;
			//dbprintf("Avg_Datagram_Period = %u us\n", Avg_Datagram_Period);
		}
	}
	else {
		// missed a datagram : restart avg computation
		First_datagram_time=0;
		First_datagram_number=Msg_Datagram.number;
		dbprintln("synchronizing");
	}
	Previous_datagram_number=Msg_Datagram.number;
}

void Transceiver::PrintMsgDatagram(MsgDatagram datagram) {
	dbprintf("%04x T%x ", datagram.number, datagram.type);
	for (uint8_t idx=0; idx<MSGVALUES; idx++)
		dbprintf("x%04x ", datagram.message[idx]);
}

void Transceiver::PrintAckDatagram(AckDatagram datagram) {
	dbprintf("%04x T%x ", datagram.number, datagram.type);
	for (uint8_t idx=0; idx<ACKVALUES; idx++)
		dbprintf("x%04x ", datagram.message[idx]);
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
	arrange_values(GetSessionKey(), DEF_MAXCHAN, DEF_MONOCHAN, MonoChannel, sizeof(RF24Channels), RF24Channels);
	/*
	dbprintf("AssignChannels(%d)\n", GetSessionKey());
	for (uint8_t idx=0; idx<sizeof(RF24Channels); idx++) {
		dbprintf("%02d ",RF24Channels[idx]);
		if (idx%10 == 9)
			dbprint('\n');
	}
	dbprint('\n');
	*/
}

// Use the radio channel corresponding to given datagram number
uint8_t Transceiver::SetChannel(uint16_t dg_number) {
	uint8_t retval=RF24Channels[dg_number % sizeof(RF24Channels)];
    Radio_obj.setChannel(retval);
	//dbprintf("SetChannel(%d) returns %d\n", dg_number, retval);
	return retval;
}

void Transceiver::SetPaLevel(int value) {
    Radio_obj.setPALevel(value);
}

// Obtain the current value of the TransmissionId, as a \0 terminated string
// size of value_out = sizeof(TransmissionId)+1
void Transceiver::GetTransmissionId(char *value_out) {
	memcpy(value_out, TransmissionId, sizeof(TransmissionId));
	value_out[sizeof(TransmissionId)]='\0';
}

// Set the TransmissionId using given device_id
// $$TODO assign it to the tx and rx pipes in Config() ?
void Transceiver::SetTransmissionId(uint16_t device_id) {
	char transid_str[sizeof(TransmissionId)+1];
	snprintf(transid_str, sizeof(transid_str), "x%04X", device_id);
	memcpy(TransmissionId, transid_str, sizeof(TransmissionId));
}

// Acquire the transmitter's configuration settings
// method called by the receiver only
// void Transceiver::AcquireTransmissionSettings(uint16_t tx_device_id, uint16_t pa_level, uint16_t session_key) {
// 	SetTransmissionId(tx_device_id);
// 	SetSessionKey(session_key);
// 	Radio_obj.setPALevel(pa_level, true);
// }

uint16_t Transceiver::GetSessionKey(void) {
	return Session_key;
}
void Transceiver::SetSessionKey(uint16_t key) {
	Session_key=key;
}

