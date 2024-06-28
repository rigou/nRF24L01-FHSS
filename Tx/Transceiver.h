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

#pragma once
#include <SPI.h>
#include <RF24.h>
#include "Common.h"
#include "rgRng.h"

typedef unsigned long micros_t; // custom name for data type suitable for times in microseconds

class Transceiver {
        
    public:
        // the Tx device identifier used while pairing devices
		static const uint16_t DEF_TXID=0x2401;

        // the channel used for transmitting in single frequency while pairing devices
        static const uint8_t DEF_MONOCHAN=64;
 
        // channel values: 0-125, actual frequency=2400+channel, eg 2483 MHz for channel 83
        // your local laws may not allow the full frequency range (authorized frequencies in France: 2400-2483.5 MHz)
        static const uint8_t DEF_MAXCHAN=COM_MAXCHAN; // channel 0 to channel DEF_MAXCHAN = DEF_MAXCHAN+1 consecutive channels

        // the default Amplifier (PA) level and Low Noise Amplifier (LNA) state
		// values: RF24_PA_MIN (0), RF24_PA_LOW (1), RF24_PA_HIGH (2), RF24_PA_MAX (3) ; definition at line 35 of file RF24.h
        static const uint8_t DEF_PALEVEL=RF24_PA_MIN; // 0

        // The (Msg/Ack)Datagram.type is a combination of these bits
        static const uint8_t DGT_SERVICE=0x1;
        static const uint8_t DGT_USER=0x2;
        static const uint8_t DGT_SYNCHRONIZED=0x4;
        static const uint8_t DGT_PAIRING_INPROGRESS=0x8;
        static const uint8_t DGT_PAIRING_COMPLETE=0x10;
     
        static const uint8_t MSGVALUES=COM_MSGVALUES; // min=4, max=14 (4 values are used by service datagrams sent by Tx while running in MONOFREQ mode)
        struct MsgDatagram {
            uint16_t number; // 0 - 65535
            uint16_t type;  // see above
            uint16_t message[MSGVALUES];
        };
        MsgDatagram Msg_Datagram; // sent from Tx -> Rx

        static const uint8_t ACKVALUES=COM_ACKVALUES; // min=1, max=14 (message[0] is used by ACK datagrams sent by Rx with status DGT_SYNCHRONIZED)
        struct AckDatagram {
            uint16_t number; // 0 - 65535
            uint16_t type;  // see above
            uint16_t message[ACKVALUES];
        };
        AckDatagram Ack_Datagram; // sent from Rx -> Tx

        micros_t Avg_Datagram_Period=0; // microsec, computed by Receive()

        Transceiver();
        bool Config(bool is_tx, uint16_t tx_device_id, uint16_t mono_channel, uint16_t pa_level);
        void HotConfig(bool is_tx, uint16_t tx_device_id, uint16_t mono_channel, uint16_t pa_level);
        bool Send(uint16_t msg_type, uint16_t *message);
        bool Receive(uint16_t ack_type, uint16_t *ack_message);
        void PrintMsgDatagram(MsgDatagram datagram);
        void PrintAckDatagram(AckDatagram datagram);
        void AssignChannels(void);
        uint8_t SetChannel(uint16_t dg_number);
        void SetPaLevel(int value);
        uint16_t GetSessionKey(void);
        void SetSessionKey(uint16_t key);
        void GetTransmissionId(char *value_out);
        void SetTransmissionId(uint16_t device_id);

    private:
        
        RF24 Radio_obj;

        // SPI configuration
        // see Gpio.h for SPI gpios assignments
        static const unsigned int SPI_SPEED=2000000; // Hz, see ARDUINO/Espressif/SPI_MasterSlave/doc/spi_clock_frequency_information.txt

        // This string uniquely identifies the data stream between the transmitter and the receiver
        // if another pair of transmitter/receiver is running in the neighbourhood, they should use a different identifier
        // value is set by Config(), either derived from DEF_TXID or read in Settings (key "TRANSID")
        uint8_t TransmissionId[5];
    
        // the channel used for transmitting in single frequency, it is set by Config()
        // it does not belong to the RF24Channels[] array, it is used by AssignChannels()
        // value is either DEF_MONOCHAN or read in Settings (key "SINFRCHAN")
        uint8_t MonoChannel=0;

        // the default Amplifier (PA) level and Low Noise Amplifier (LNA) state
        static const rf24_pa_dbm_e DEFAULT_PA_LEVEL=RF24_PA_MIN;

        // a randomized array of all radio channels used for frequency hopping
        // MonoChannel and DEF_MONOCHAN are not in the array
        uint8_t RF24Channels[DEF_MAXCHAN-1];
        uint16_t Session_key=0; // seed used to randomize the RF24Channels[] array

        rgRng Random_obj;

        void compute_avg_datagram_period(void);
        void arrange_values(const unsigned int key, const uint8_t max_value, const uint8_t ignored_value1, const uint8_t ignored_value2, const uint8_t sizeof_values_out, uint8_t *values_out);

};
