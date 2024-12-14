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

#include <rgBtn.h>
#include <rgCsv.h>
#include "Common.h"
#include "Gpio.h"
#include "Settings.h"
#include "Transceiver.h"
#include "User.h"

// 0=debug off, 1=output to serial, 2=output to serial and optionally bluetooth with dbtprintln()
#define DEBUG_ON 1
// 0=trace off, 1=output to serial, 2=output to serial and optionally bluetooth with trbtprintln()
#define TRACE_ON 0
#include <rgDebug.h>

Settings Settings_obj;
Transceiver Transceiver_obj;

#define APP_NAME "BasicRx"
#define APP_VERSION "1.9.1"

// Debug stuff
//
// Printing datagrams consumes 2300 µs
//#define DEBUG_PRINT_MSG_DATAGRAMS 
//#define DEBUG_PRINT_ACK_DATAGRAMS

// SYNCHRONIZING : we compute the datagram period then we switch to MONOFREQ
// MONOFREQ : we acquire the transmitter's configuration settings and :
//  if we were not listening on DEF_MONOCHAN we switch to MULTIFREQ
//  if we were listening on DEF_MONOCHAN we save the transmitter's configuration settings in our settings file, and reboot
// MULTIFREQ : we process the user datagram and send back the ACK datagrams
enum RxStates {SYNCHRONIZING, MONOFREQ, MULTIFREQ};
RxStates Rx_state=SYNCHRONIZING;

bool RunLedEnabled=(RUNLED_GPIO!=0); // true by default, false when the Pairing button is pressed

uint16_t Ack_message[Transceiver::ACKVALUES];
uint16_t Ack_type=Transceiver::DGT_SERVICE;

uint16_t ErrorCounter=0;  // number of missing datagrams per second, updated once/second, available to and used by User.cpp

// while MONOFREQ, we will send SYNACKNUMBER ack datagrams informing Tx that we are synchronized,
// then we will transition to MULTIFREQ after sending the last of them.
const uint8_t SYNACKNUMBER=64;

void setup() {
    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect
#if DEBUG_ON
	dbprint('\n'); for (uint8_t idx = 0; idx<6; idx++) { dbprint((char)('A'+idx)); delay(500); }
#endif
    dbprintf("\n\n*** %s %s() : %s %s\n", __FILE_NAME__, __FUNCTION__, APP_NAME, APP_VERSION);
    dbprintf("using library %s %s\n", BTNLIB_NAME, BTNLIB_VERSION);
    dbprintf("using library %s %s\n", CSVLIB_NAME, CSVLIB_VERSION);
    pinMode(PAIRING_GPIO, INPUT_PULLUP);
    if (RUNLED_GPIO)
        pinMode(RUNLED_GPIO, OUTPUT);
    if (ERRLED_GPIO)
        pinMode(ERRLED_GPIO, OUTPUT);
    digitalWrite(RUNLED_GPIO, HIGH);

    // mount the file system, format it if not already done
    if (!LittleFS.begin(true))
		EndProgram(true, "Setup: file system formatting error");

    // Hold the Pairing button during boot to delete the current settings file.
    const int BUTTON_HOLD=3000; // ms, hold the button long enough to create a new file with default values
    BtnStates btn_state=ReadBtnBlock(PAIRING_GPIO, RUNLED_GPIO, BUTTON_HOLD); // ms
    if (btn_state==BTN_REACHED_DURATION) {
        if (LittleFS.exists(Settings_obj.PARFILE)) {
            if (LittleFS.remove(Settings_obj.PARFILE))
                dbprintln("Setup: settings file deleted");
            else
                dbprintln("Setup: error deleting settings file"); // continue anyway
        }
        else
            dbprintf("Setup: settings file \"%s\" not found\n", Settings_obj.PARFILE);
    }
 
    // Open the settings file, create it with given parameters if it does not exist
    // these parameters will make us enter the pairing mode automatically
    // after synchronization is complete, without further action on the Pairing button 
    if (Settings_obj.Init(Transceiver::DEF_TXID, Transceiver::DEF_RXID, Transceiver::DEF_MONOCHAN, Transceiver::DEF_PALEVEL)!=0)
        EndProgram(true, "Setup: Settings file error");

    // read the transceiver settings from the settings file
    int tx_device_id=Settings_obj.GetTxDeviceId();
    int rx_device_id=Settings_obj.GetRxDeviceId();
    int mono_channel=Settings_obj.GetMonoChannel();
    int pa_level=Settings_obj.GetPaLevel();
    if (tx_device_id < 0 || rx_device_id < 0 || mono_channel<0 || pa_level<0) {
        const char *message=NULL;
        if (tx_device_id < 0)
            message="tx_device_id not found";
        if (rx_device_id < 0)
            message="rx_device_id not found";
        if (mono_channel<0)
            message="mono_channel not found";
        if (pa_level<0)
            message="pa_level not found";
        EndProgram(true, message);
    }

    // Configure the transceiver
    Transceiver_obj.Setup(false, tx_device_id, rx_device_id, mono_channel, pa_level);

    // Run user setup code
    UserSetup();

    trprintf("*** %s %s() returns after %lu ms\n", __FILE_NAME__, __FUNCTION__, millis());
}

void loop() {
    // micros_t start_timer = micros();
    // receive a datagram
    uint8_t result=receive();

    if (result==0) { // 0=received a DGT_USER datagram
        UserLoopMsg(Transceiver_obj.Msg_Datagram.message);
        Ack_type=Transceiver::DGT_USER;
        memset(Ack_message, 0, sizeof(Ack_message));
        UserLoopAck(Ack_message);
        //dbprintf("Receive time=%lu\n", micros() - start_timer);
    }
    
    if (Rx_state==MULTIFREQ) {
        if (RunLedEnabled)
            BlinkLed(RUNLED_GPIO, 2000, 20, false); // short flash once / every 2 seconds (0.5 Hz)
        if (result==3)
            FlashLed(ERRLED_GPIO, 20); // missed a datagram (timeout) : turn on ERRLED_GPIO
        else
            FlashLed(ERRLED_GPIO); // refresh ERRLED_GPIO
    }
    else {
        if (RunLedEnabled)
            BlinkLed(RUNLED_GPIO, 500, 200, false); // longer flash twice / second (2 Hz)
    }
}

// Return values: 
//  0=received a user datagram
//  1=received a service datagram
//  2=synchronization in progress
//  3=missed a datagram (timeout)
//  4=no datagram available in the reception pipe
uint8_t receive(void) {
    #define pairing_in_progress (Transceiver_obj.GetChannel()==Transceiver::DEF_MONOCHAN)

    uint8_t retval=2;
    // next datagram will arrive between Next_Eta_us and Next_Eta_us+Avg_Datagram_Period
    // Next_Eta_us is the moment where we switch to next radio channel
    bool received=false;
    bool timeout=false;
    static micros_t Next_Eta_us=0;
    static uint16_t Prev_number=0;
    static uint16_t Multifreq_number=0; // we'll start frequency hopping *after* receiving this datagram
    static uint16_t Err_count=0;  // number of missing datagrams per second, updated once/second
    //static uint16_t Reset_counter=0;  // number of consecutive missing datagrams while MULTIFREQ

    unsigned long time_now_ms=millis();
    static unsigned long Err_timer=0; // to update the global ErrorCounter once/second
    static unsigned long Sig_timer=0; // to print "no signal" warning every second

    if (time_now_ms >= Err_timer+1000) {
        ErrorCounter=Err_count; // update the global ErrorCounter once/second
        Err_count=0; // reset the local error counter
        Err_timer=time_now_ms;
    }
    if (time_now_ms >= Sig_timer+1000) {
        dbprintf("(ch 0x%02x) tx_device_id=0x%06x rx_device_id=0x%06x dg_number=%u no signal\n", 
            Transceiver_obj.GetChannel(), Settings_obj.GetTxDeviceId(), Settings_obj.GetRxDeviceId(), Transceiver_obj.Msg_Datagram.number);
        Sig_timer=time_now_ms;
    }

    // receive next datagram and send the ACK datagram waiting in the Ack buffer
    micros_t time_now_us=micros();
    if (Transceiver_obj.Receive(Ack_type, Ack_message)) {
        Sig_timer=time_now_ms; // to print "no signal" warning every second 
        received=true;
        memset(Ack_message, 0, sizeof(Ack_message)); // clear the Ack buffer for the next ACK datagram

#ifdef DEBUG_PRINT_MSG_DATAGRAMS
        dbprint("Msg: ");
        Transceiver_obj.PrintMsgDatagram(Transceiver_obj.Msg_Datagram);
        dbprint('\n');
#endif
#ifdef DEBUG_PRINT_ACK_DATAGRAMS
        dbprint("Ack: ");
        Transceiver_obj.PrintAckDatagram(Transceiver_obj.Ack_Datagram);
        dbprint('\n');
#endif
    }

    if (received) {
        if (Rx_state==SYNCHRONIZING) {
            // the transmitter is sending MSG datagrams containing the its configuration settings
            Ack_type=Transceiver::DGT_SERVICE; // ack datagrams sent while SYNCHRONIZING are empty
            if (Transceiver_obj.Avg_Datagram_Period) {
                Rx_state=MONOFREQ;
                // send SYNACKNUMBER ack datagrams informing Tx that we are synchronized and will switch to MULTIFREQ at this datagram number
                Multifreq_number=Transceiver_obj.Msg_Datagram.number+SYNACKNUMBER;
                dbprintf("synchronized after %lu ms, period=%lu µs\n", millis(), Transceiver_obj.Avg_Datagram_Period);
            }
            retval=2; // synchronization in progress
        }

        if (Rx_state==MONOFREQ || Rx_state==MULTIFREQ) {
            Next_Eta_us=time_now_us+Transceiver_obj.Avg_Datagram_Period;
            Prev_number=Transceiver_obj.Msg_Datagram.number;
            if (Rx_state==MONOFREQ) {
                // the transmitter is still sending MSG datagrams containing its configuration settings
                Ack_type=Transceiver::DGT_SERVICE | Transceiver::DGT_SYNCHRONIZED;
                if (pairing_in_progress)
                    Ack_type|=Transceiver::DGT_PAIRING;
                Ack_message[0]=Multifreq_number;
                Ack_message[1]=Transceiver_obj.GetSessionKey(); // $$DEBUG
            }
            else {
                // MULTIFREQ
                if (Transceiver_obj.Msg_Datagram.type & Transceiver::DGT_USER)
                    retval=0; // received user datagram
                else
                    retval=1; // received service datagram
                //Reset_counter=0;
            }
        }
    }
    else {
        // not received
        if ((Rx_state==MONOFREQ || Rx_state==MULTIFREQ)) {
            if (time_now_us>=Next_Eta_us+(Transceiver_obj.Avg_Datagram_Period/2)) {
                //dbprintf("Mis: n=%05u, timeout=%d\n", expected_number, time_now_us-Next_Eta_us);               
                // timeout : increment Next_Eta_us blindly
                Next_Eta_us+=Transceiver_obj.Avg_Datagram_Period;
                Prev_number++;
                Err_count++;
                timeout=true;
                retval=3; // timeout : no datagram received in the expected time slice
            }
            else {
                timeout=false;
                retval=4; // no datagram available yet in the reception pipe
            }

        }
    }
        
    if (received||timeout) { // else still waiting for next datagram
        if (Rx_state==MONOFREQ) {
            static bool Acquired_tx_config=false;
            if (Prev_number != Multifreq_number) {
                if (!Acquired_tx_config) {
                    // Acquire the transmitter's configuration settings
                    uint16_t tx_device_id=Transceiver_obj.Msg_Datagram.message[0];
                    uint16_t rx_device_id=Transceiver_obj.Msg_Datagram.message[1];
                    uint16_t mono_channel=Transceiver_obj.Msg_Datagram.message[2];
                    uint16_t pa_level=Transceiver_obj.Msg_Datagram.message[3];
                    uint16_t session_key=Transceiver_obj.Msg_Datagram.message[4];
                    // Register the session key in RAM in order to use it later for frequency hopping
                    Transceiver_obj.SetSessionKey(session_key); // random seed used to generate the RF24Channels[] array
                    Transceiver_obj.AssignChannels(); // assign values to the array of radio channels
                    // apply received pa_level
                    Transceiver_obj.SetPaLevel(pa_level);
                    bool save_settings=false;
                    if (pairing_in_progress) {
                        // Pairing : acquire the configuration settings received from the transmitter
                        Settings_obj.SetTxDeviceId(tx_device_id);
                        Settings_obj.SetRxDeviceId(rx_device_id);
                        Settings_obj.SetMonoChannel(mono_channel);
                        save_settings=true;
                    }
                    // Persist the pa_level received from the transmitter
                    if (Settings_obj.GetPaLevel()!=pa_level) {
                        Settings_obj.SetPaLevel(pa_level);
                        save_settings=true;
                    }
                    if (save_settings) {
                        if (Settings_obj.Save()<0)
                            EndProgram(true, "Settings write error"); // false=halt command, could not save settings
                    }
                    Acquired_tx_config=true;
                }
            }
            else {
                if (pairing_in_progress) {
                    dbprintln("Reboot after pairing");
                    EndProgram(true); // true=reset command
                }

                Rx_state=MULTIFREQ;
                dbprintf("MULTIFREQ after %lu ms, period=%lu µs\n", millis(), Transceiver_obj.Avg_Datagram_Period);
            }
        }
            
        if (Rx_state==MULTIFREQ) {
            // switch to next radio channel
            Transceiver_obj.SetChannel(Prev_number+1);
        }
    } // if (received||timeout)
    return retval;
}

