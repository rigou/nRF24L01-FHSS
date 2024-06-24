/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#pragma once
#include <SPI.h>
#include <RF24.h>
#include "rgRng.h"

typedef unsigned long micros_t; // custom name for data type suitable for times in microseconds

class Transceiver {
        
    public:
        // the Tx device identifier used while pairing devices
		static const uint16_t DEF_TXID=0x2401;

        // the channel used for transmitting in single frequency while pairing devices
        static const byte DEF_MONOCHAN=64;
 
        // channel values: 0-125, actual frequency=2400+channel, eg 2483 MHz for channel 83
        // your local laws may not allow the full frequency range (authorized frequencies in France: 2400-2483.5 MHz)
        static const byte DEF_MAXCHAN=83; // channel 0 to channel 83 = 84 consecutive channels

        // the default Amplifier (PA) level and Low Noise Amplifier (LNA) state
		// values: RF24_PA_MIN (0), RF24_PA_LOW (1), RF24_PA_HIGH (2), RF24_PA_MAX (3) ; definition at line 35 of file RF24.h
        static const byte DEF_PALEVEL=RF24_PA_MIN; // 0

        // The (Msg/Ack)Datagram.type is a combination of these bits
        static const byte DGT_SERVICE=0x1;
        static const byte DGT_USER=0x2;
        static const byte DGT_SYNCHRONIZED=0x4;
        static const byte DGT_PAIRING_INPROGRESS=0x8;
        static const byte DGT_PAIRING_COMPLETE=0x10;
     
        static const byte MSGVALUES=10; // min=4, max=14 (4 values are used by service datagrams sent by Tx while running in MONOFREQ mode)
        struct MsgDatagram {
            uint16_t number; // 0 - 65535
            uint16_t type;  // see above
            uint16_t message[MSGVALUES];
        };
        MsgDatagram Msg_Datagram; // sent from Tx -> Rx

        static const byte ACKVALUES=5; // min=1, max=14 (message[0] is used by ACK datagrams sent by Rx with status DGT_SYNCHRONIZED)
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
        byte SetChannel(uint16_t dg_number);
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
        byte TransmissionId[5];
    
        // the channel used for transmitting in single frequency, it is set by Config()
        // it does not belong to the RF24Channels[] array, it is used by AssignChannels()
        // value is either DEF_MONOCHAN or read in Settings (key "SINFRCHAN")
        byte MonoChannel=0;

        // the default Amplifier (PA) level and Low Noise Amplifier (LNA) state
        static const rf24_pa_dbm_e DEFAULT_PA_LEVEL=RF24_PA_MIN;

        // a randomized array of all radio channels used for frequency hopping
        // MonoChannel and DEF_MONOCHAN are not in the array
        byte RF24Channels[DEF_MAXCHAN-1];
        uint16_t Session_key=0; // seed used to randomize the RF24Channels[] array

        rgRng Random_obj;

        void compute_avg_datagram_period(void);
        void arrange_values(const unsigned int key, const byte max_value, const byte ignored_value1, const byte ignored_value2, const byte sizeof_values_out, byte *values_out);

};
