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
#include "Settings.h"
#include "Transceiver.h"
#include <rgButton.h>
#include "User.h"

Settings Settings_obj;
Transceiver Transceiver_obj;

#define APP_NAME "Rx"
#define APP_VERSION "1.6.2"

// Debug stuff
//
// Simulate Rx random disconnections to test automatic connection recovery
//#define DEBUG_RANDOM_DISCONNECT
//
// Printing datagrams consumes 2300 µs
//#define DEBUG_PRINT_MSG_DATAGRAMS 
//#define DEBUG_PRINT_ACK_DATAGRAMS

// Tx and Rx start in monofrequency and they both switch to MULTIFREQ after Rx tells Tx that it is synchronized
// Avg_Datagram_Period is computed while SYNCHRONIZING in monofrequency, and no useful data is transmitted
// Pairing may be started in MONOFREQ or MULTIFREQ, which sets the state back to MONOFREQ, and is actually performed while MONOFREQ
// User data is transmitted while MULTIFREQ
// notice: SYNCHRONIZING takes place while Rx is in monofrequency. Rx state switches to MONOFREQ when synchronization is complete.
enum RxStates {SYNCHRONIZING, MONOFREQ, MULTIFREQ};
RxStates Rx_state=SYNCHRONIZING; // while PairingInProgress state MULTIFREQ is never reached 

bool RunLedEnabled=(RUNLED_GPIO!=0); // true by default, false when the Pairing button is pressed

uint16_t Ack_message[Transceiver::ACKVALUES];
uint16_t Ack_type=Transceiver::DGT_SERVICE;

uint16_t ErrorCounter=0;  // number of missing datagrams per second, updated once/second

bool PairingInProgress=false; // set true by setup() if settings key PAIREQ=1
bool PairingComplete=false; // set true by receive()
micros_t RebootTime=0; // we'll reboot at this moment, after pairing is complete
    
// while MONOFREQ, we will send SYNACKNUMBER ack datagrams informing Tx that we are synchronized,
// then we will transition to MULTIFREQ after sending the last of them.
const uint8_t SYNACKNUMBER=64;

// #define SCOPE_GPIO 17 // the oscilloscope probe

void setup() {
    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect
	//Serial.print('\n'); for (uint8_t idx = 0; idx<4; idx++) { Serial.print((char)('A'+idx)); delay(500); } // debug
    Serial.printf("\n\n%s %s\n", APP_NAME, APP_VERSION);

    // pinMode(SCOPE_GPIO, OUTPUT);
    pinMode(PAIRING_GPIO, INPUT_PULLUP);
    if (RUNLED_GPIO)
        pinMode(RUNLED_GPIO, OUTPUT);
    if (ERRLED_GPIO)
        pinMode(ERRLED_GPIO, OUTPUT);

    // Open the settings file, create it if it does not exist
    // default values are provided only for creating the settings file
    if (Settings_obj.Init(PAIRING_GPIO, RUNLED_GPIO, Transceiver::DEF_TXID, Transceiver::DEF_MONOCHAN, Transceiver::DEF_PALEVEL)!=0)
        EndProgram(false); // halt command

    // read the transceiver settings
    int device_id=Settings_obj.GetDeviceId();
    int mono_channel=Settings_obj.GetMonoChannel();
    int pa_level=Settings_obj.GetPaLevel();
    if (device_id < 0 || mono_channel<0 || pa_level<0)
        EndProgram(false); // halt command

    // Configure the transceiver
    Transceiver_obj.Config(false, device_id, mono_channel, pa_level);

    // Run user setup code
    UserSetup();

    Serial.printf("Starting after %lu ms\n", millis());
}

void loop() {
    // micros_t start_timer = micros();
    // if (Transceiver_obj.Radio_obj.available())
    //     digitalWrite(SCOPE_GPIO, HIGH);

    uint8_t result=receive();
    // digitalWrite(SCOPE_GPIO, LOW);

    if (result==0) { // 0=received a DGT_USER datagram
        UserLoopMsg(Transceiver_obj.Msg_Datagram.message);
        Ack_type=Transceiver::DGT_USER;
        memset(Ack_message, 0, sizeof(Ack_message));
        UserLoopAck(Ack_message);
        //Serial.printf("Receive time=%lu\n", micros() - start_timer);
    }
    
    if (Rx_state==MULTIFREQ) {
        if (RunLedEnabled)
            BlinkLed(RUNLED_GPIO, 1000, 20, false);
        if (result==3)
            FlashLed(ERRLED_GPIO, 20); // turn on ERRLED_GPIO
        else
            FlashLed(ERRLED_GPIO); // refresh ERRLED_GPIO
    }
    else {
        if (RunLedEnabled) {
            if (PairingInProgress)
                BlinkLed(RUNLED_GPIO, 3000, 1000, false);
            else
                BlinkLed(RUNLED_GPIO, 100, 50, false);
        }
        if (result==2 && ERRLED_GPIO)
            digitalWrite(ERRLED_GPIO, HIGH);
    }
    if (RebootTime && millis()>=RebootTime) {
        Serial.println("Delayed reboot after pairing");
        EndProgram(true); // reset command
    }

}

// Return values: 
//  0=received a user datagram
//  1=received a service datagram
//  2=synchronization in progress
//  3=missed a datagram (timeout)
uint8_t receive(void) {
    uint8_t retval=2;
    // next datagram will arrive between Next_Eta_us and Next_Eta_us+Avg_Datagram_Period
    // Next_Eta_us is the moment where we switch to next radio channel
    bool received=false;
    bool timeout=false;
    static micros_t Next_Eta_us=0;
    static uint16_t Prev_number=0;
    static uint16_t Multifreq_number=0; // we'll start frequency hopping *after* receiving this datagram
    static uint16_t Reset_counter=0;  // number of consecutive missing datagrams while MULTIFREQ: we reset the MCU if this counter reaches 100
    static unsigned long Stat_time_ms=0; // to compute error statistics every second
    static unsigned long Last_signal_time_ms=millis(); // to print "no signal" warning every second
    static uint16_t Error_counter=0;  // number of missing datagrams per second, updated once/second
    
    unsigned long time_now_ms=millis();
    if (Rx_state==SYNCHRONIZING) {
        if (time_now_ms >= Last_signal_time_ms+1000) {
            Serial.println("no signal");
            Last_signal_time_ms=time_now_ms;
        }
    }
    else {
        // clear error statistics every second
        if (Stat_time_ms && time_now_ms >= Stat_time_ms+1000) {
            if (Rx_state==MULTIFREQ && Transceiver_obj.Avg_Datagram_Period) {
                ErrorCounter=Error_counter; // update the global ErrorCounter once/second
                Error_counter=0;
                Stat_time_ms=time_now_ms;
            }
        }
    }
    
    micros_t time_now_us=micros();
    if (Transceiver_obj.Receive(Ack_type, Ack_message)) {
        memset(Ack_message, 0, sizeof(Ack_message));
        if (Rx_state==SYNCHRONIZING) {
            Ack_type=Transceiver::DGT_SERVICE; // ack datagrams sent while SYNCHRONIZING are empty
            if (Transceiver_obj.Avg_Datagram_Period) {
                Rx_state=MONOFREQ;
                Serial.printf("synchronized after %lu ms, period=%lu µs\n", millis(), Transceiver_obj.Avg_Datagram_Period);
                // send SYNACKNUMBER ack datagrams informing Tx that we are synchronized and will switch to MULTIFREQ at this datagram number
                Multifreq_number=Transceiver_obj.Msg_Datagram.number+SYNACKNUMBER;
            }
            retval=2;
        }
        Last_signal_time_ms=time_now_ms; // to print "no signal" warning every second 
        received=true;
    }
#if defined(DEBUG_PRINT_MSG_DATAGRAMS) || defined(DEBUG_PRINT_ACK_DATAGRAMS)
    if (received) {
#ifdef DEBUG_PRINT_MSG_DATAGRAMS
        Serial.print("Msg: ");
        Transceiver_obj.PrintMsgDatagram(Transceiver_obj.Msg_Datagram);
        Serial.print('\n');
#endif
#ifdef DEBUG_PRINT_ACK_DATAGRAMS
        Serial.print("Ack: ");
        Transceiver_obj.PrintAckDatagram(Transceiver_obj.Ack_Datagram);
        Serial.print('\n');
#endif
    }
#endif
    // check actions on the Pairing button
    if (Rx_state==SYNCHRONIZING) { // else already paired
        BtnStates btn_state=ReadBtn(PAIRING_GPIO, RUNLED_GPIO, 1500);
        RunLedEnabled=(btn_state==BTN_RELEASED); // if btn_state==BTN_PRESSED then read_button() will control the Led
        if (btn_state==BTN_REACHED_DURATION) {
            // blink led for 3 seconds to tell user that he can release the button
            unsigned long stop_time=millis()+3000;
            while (millis()<stop_time)
                BlinkLed(RUNLED_GPIO, 100, 50, false);
            digitalWrite(RUNLED_GPIO, LOW);

            // reconfigure the transceiver for pairing
            Serial.println("Pairing");
            PairingInProgress=true;
            Transceiver_obj.HotConfig(false, Transceiver::DEF_TXID, Transceiver::DEF_MONOCHAN, Transceiver::DEF_PALEVEL);
            Rx_state=MONOFREQ;
        }
    }
    
    if (Rx_state==MONOFREQ || Rx_state==MULTIFREQ) {
        if (received) {
            Next_Eta_us=time_now_us+Transceiver_obj.Avg_Datagram_Period;
            Prev_number=Transceiver_obj.Msg_Datagram.number;
            if (Rx_state==MONOFREQ) {
                Ack_type=Transceiver::DGT_SERVICE | Transceiver::DGT_SYNCHRONIZED;
                if (PairingComplete)
                    Ack_type |= Transceiver::DGT_PAIRING_COMPLETE;
                Ack_message[0]=Multifreq_number;
            }
            else {
                // MULTIFREQ
                if (Transceiver_obj.Msg_Datagram.type & Transceiver::DGT_USER)
                    retval=0; // received user datagram
                else
                    retval=1; // received service datagram
                Reset_counter=0;
            }
        }
        else if (time_now_us>=Next_Eta_us+(Transceiver_obj.Avg_Datagram_Period/2)) {
            //Serial.printf("Mis: n=%05u, timeout=%d\n", expected_number, time_now_us-Next_Eta_us);
            if (Rx_state==MULTIFREQ) {
                // reset if we counted 100 consecutive missing datagrams (Tx was turned off ?)
                Reset_counter++;
                if (Reset_counter==100) {
                    //Settings_obj.Save();
                    Serial.println("Reboot after 100 consecutive missed datagrams");
                    EndProgram(true); // reset command
                }
            }
            // timeout : increment Next_Eta_us blindly
            Next_Eta_us+=Transceiver_obj.Avg_Datagram_Period;
            Prev_number++;
            Error_counter++;
            timeout=true;
            retval=3;
        }
        
        if (received||timeout) { // else still waiting for next datagram
            if (Rx_state==MONOFREQ) {
                static bool Received_tx_config=false;
                if (Prev_number != Multifreq_number) {
                    // All datagrams sent by the transmitter before reaching the MULTIFREQ state have the DGT_SERVICE type
                    // but let's check it anyway
                    if (Transceiver_obj.Msg_Datagram.type & Transceiver::DGT_SERVICE) {
                        if (!Received_tx_config) {
                            // Acquire the transmitter's configuration settings
                            uint16_t device_id=Transceiver_obj.Msg_Datagram.message[0];
                            uint16_t mono_channel=Transceiver_obj.Msg_Datagram.message[1];
                            uint16_t pa_level=Transceiver_obj.Msg_Datagram.message[2];
                            uint16_t session_key=Transceiver_obj.Msg_Datagram.message[3];
                            Serial.printf("Received device_id=x%04x mono_channel=x%04x pa_level=%04x session_key=x%04x\n", 
                                Transceiver_obj.Msg_Datagram.message[0],
                                Transceiver_obj.Msg_Datagram.message[1],
                                Transceiver_obj.Msg_Datagram.message[2],
                                Transceiver_obj.Msg_Datagram.message[3]);

                            // Register the session key in RAM in order to use it later for frequency hopping
                            Transceiver_obj.SetSessionKey(session_key);
                            // apply received pa_level
                            Transceiver_obj.SetPaLevel(pa_level);

                            bool save_settings=false;
                            if (PairingInProgress) {
                                // Persist configuration settings received from the transmitter
                                Settings_obj.SetDeviceId(device_id);
                                Settings_obj.SetMonoChannel(mono_channel);
                                save_settings=true;
                            }
                            // Persist the pa_level received from the transmitter
                            if (Settings_obj.GetPaLevel()!=pa_level) {
                                Settings_obj.SetPaLevel(pa_level);
                                save_settings=true;
                            }

                            if (save_settings)
                                if (Settings_obj.Save()<0)
                                    EndProgram(false); // halt command, could not save settings

                            if (PairingInProgress) {
                                Serial.println("Pairing complete");
                                PairingComplete=true;
                                // force reboot after sending enough datagrams informing Tx that pairing is complete on our side
                                RebootTime=millis()+1000;
                             }
                            Received_tx_config=true;
#ifdef DEBUG_RANDOM_DISCONNECT
                            uint16_t debug_disconnection_seed=GetRandomInt16();
                            Serial.printf("debug: random disconnection seed=%u\n", debug_disconnection_seed);
                            randomSeed(debug_disconnection_seed);
#endif
                        }
                    }
                }
                else if (!PairingInProgress) {
                    Rx_state=MULTIFREQ;
                    Serial.printf("MULTIFREQ after %lu ms, period=%lu µs\n", millis(), Transceiver_obj.Avg_Datagram_Period);
                    // assign values to the array of radio channels
                    Transceiver_obj.AssignChannels();
                    Stat_time_ms=millis(); // to compute error statistics every second
                }
            }
            
            if (Rx_state==MULTIFREQ) {
                // switch to next radio channel
                Transceiver_obj.SetChannel(Prev_number+1);
            }
#ifdef DEBUG_RANDOM_DISCONNECT
            // reset randomly to simulate connection loss
            if (random(500000)<10) {
                // probability 1/240000 ~ 1 per 20 minutes on average at 100 dg/s = 1 / (240000 / 100 / (60 * 2))
                Serial.printf("debug: random reboot after %lu minutes\n", millis()/60000);
                Serial.println("----------------------------------------");
                EndProgram(true); // reset command
            }
#endif
        }
    }
    return retval;
}

