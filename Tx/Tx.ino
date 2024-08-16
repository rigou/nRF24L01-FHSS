/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
 * 
 * Pairing procedure :
 * 1. Turn off Rx & Tx
 * 2. Turn on Tx                - the led flashes rapidly
 * 3. Press the Pairing button  - the led turns on and after 3 s it flashes rapidly
 * 4. Release the button        - the led flashes every 3 s, Tx is ready for pairing with Rx
 * 5. Turn on Rx                - the led flashes rapidly
 * 6. Press the Pairing button  - the led turns on and after 3 s it flashes rapidly
 * 7. Release the button        - Rx starts pairing with Tx
 * 8. Rx & Tx reboot after 1 s  - both leds flash rapidly for 1 s then flash briefly once per second
 * 9. Rx & Tx are paired and transmitting data.
 * 
 * Run minicom to display the debug log issued by dbprint() :
    LOG="$HOME/Downloads/$(date '+%Y-%m-%dT%H:%M:%S')_rf24Tx.log" ; minicom -C "$LOG" -D /dev/ttyUSB0
    LOG="$HOME/Downloads/$(date '+%Y-%m-%dT%H:%M:%S')_rf24Rx.log" ; minicom -C "$LOG" -D /dev/ttyUSB1
 * type Ctrl-A U    to terminate text lines with CR+LF for proper display 
 *      you can also set in the default settings: "Configure Minicom" / "Screen and keyboard" / "Add carriage return" 
 *      then "Save setup as dfl"
 * type Ctrl-A N    to prefix each line with a timestamp
 * type Ctrl-A Z    to display minicom menu
 * type Ctrl-A X    to quit
 *
 * This program is published under the GNU General Public License. 
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

#define APP_NAME "Tx"
#define APP_VERSION "1.6.5"

// Debug stuff
//
// Simulate Rx random disconnections to test automatic connection recovery
//#define DEBUG_RANDOM_DISCONNECT
//
// Printing datagrams consumes 2300 µs
//#define DEBUG_PRINT_MSG_DATAGRAMS 
//#define DEBUG_PRINT_ACK_DATAGRAMS

// Tx and Rx start in monofrequency and they both switch to MULTIFREQ after Rx tells Tx that it is synchronized
// Pairing may be started in MONOFREQ or MULTIFREQ, which sets the state back to MONOFREQ, and is actually performed while MONOFREQ
// User data is transmitted while MULTIFREQ
enum TxStates {MONOFREQ, MULTIFREQ};
TxStates Tx_state=MONOFREQ;

bool RunLedEnabled=(RUNLED_GPIO!=0); // true by default, false when the Pairing button is pressed

uint16_t Msg_message[Transceiver::MSGVALUES];
uint16_t Msg_type=Transceiver::DGT_SERVICE;

uint16_t ErrorCounter=0;  // number of transmission errors per second, updated once/second

// DGPERIOD is the delay between 2 datagrams, in microseconds
// Acceptable CPU speed for DGPERIOD=10000 : 80-240 MHz on Tx and/or Rx
// Larger delays increase the time available for your application data processing between datagrams
const micros_t DGPERIOD=1000000/COM_TRANS_DGS; // microseconds, must be multiple of 100
bool PairingInProgress=false;

// Autorepeat timer used to transmit datagrams periodically
// see https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
hw_timer_t * Timer_obj = NULL;
volatile SemaphoreHandle_t Semaphore_obj;

void ARDUINO_ISR_ATTR onTimer(){
    xSemaphoreGiveFromISR(Semaphore_obj, NULL);
}

// #define SCOPE_GPIO 17 // the oscilloscope probe

void setup() {
    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect   
	//dbprint('\n'); for (uint8_t idx = 0; idx<4; idx++) { dbprint((char)('A'+idx)); delay(500); } // debug
    dbprintf("\n\n%s %s\n", APP_NAME, APP_VERSION);

    // pinMode(SCOPE_GPIO, OUTPUT);
    pinMode(PAIRING_GPIO, INPUT_PULLUP);
    if (RUNLED_GPIO)
        pinMode(RUNLED_GPIO, OUTPUT);
    if (ERRLED_GPIO)
        pinMode(ERRLED_GPIO, OUTPUT);
	pinMode(PALEVEL0_GPIO, INPUT_PULLUP);
	pinMode(PALEVEL1_GPIO, INPUT_PULLUP);
    digitalWrite(RUNLED_GPIO, HIGH);
                
    // Open the settings file, create it if it does not exist
    // default values are provided only for creating the settings file
    if (Settings_obj.Init(PAIRING_GPIO, RUNLED_GPIO, Transceiver::DEF_TXID, Transceiver::DEF_MONOCHAN, Transceiver::DEF_PALEVEL)!=0)
        EndProgram(false); // halt command

    // read the transceiver settings
    int device_id=(uint16_t)Settings_obj.GetDeviceId();
    int mono_channel=Settings_obj.GetMonoChannel();
    if (device_id < 0 || mono_channel<0)
        EndProgram(false); // halt command
    int pa_level=read_pa_level_switch(PALEVEL0_GPIO, PALEVEL1_GPIO);

    if (device_id==Transceiver::DEF_TXID) {
        // if this is the first boot ever, assign random values to settings "TXID" and "MONOCHAN"
        device_id=GetRandomInt16();
        mono_channel=Transceiver::DEF_MONOCHAN;
        while (mono_channel==Transceiver::DEF_MONOCHAN)
            mono_channel=GetRandomInt16()%(Transceiver::DEF_MAXCHAN+1);
        Settings_obj.SetDeviceId(device_id);
        Settings_obj.SetMonoChannel(mono_channel);
        Settings_obj.Save();
    }
    
    // Configure the transceiver
    Transceiver_obj.Config(true, device_id, mono_channel, pa_level);

    // clear data in the MSG message buffer : datagram number 0 will contain only zeros
    memset(Msg_message, 0, sizeof(Msg_message));

    // this semaphore tells when the timer has fired
    // we use it to make the onTimer() ISR return as fast as possible
    Semaphore_obj = xSemaphoreCreateBinary();
    // Set timer frequency to 10 kHz to get a 100 microsecond resolution (slower frequency won't work)
    Timer_obj = timerBegin(10000);
    // Attach onTimer function to our timer.
    timerAttachInterrupt(Timer_obj, &onTimer);
    // Set alarm to call onTimer function every DGPERIOD microseconds
    // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter)
    // onTimer() will be called when the counter of Timer_obj reaches this value, and the counter will be reset to 0
    timerAlarm(Timer_obj, DGPERIOD/100, true, 0);

    // Run user setup code
    UserSetup();

    dbprintf("Starting after %lu ms\n", millis());
}

void loop() {
    bool result=true;
    if (xSemaphoreTake(Semaphore_obj, 0) == pdTRUE) {
        // micros_t start_timer = micros();
        
        // datagrams transmitted before reaching the MULTIFREQ state 
        // are reserved for the internal workings of this program
        // you cannot use them
        if (Tx_state==MULTIFREQ) {
            Msg_type=Transceiver::DGT_USER;
            memset(Msg_message, 0, sizeof(Msg_message));
            UserLoopMsg(Msg_message);
        }
        else {
            // datagrams transmitted before reaching the MULTIFREQ state are Service datagrams,
            // they are reserved for the internal workings of this program, and you cannot use them
            // do not change anything here
            Msg_type=Transceiver::DGT_SERVICE;
            // notice: 1st Service datagram has no contents
        }

        // digitalWrite(SCOPE_GPIO, HIGH);
        result=send();
        // digitalWrite(SCOPE_GPIO, LOW);
#ifdef DEBUG_PRINT_MSG_DATAGRAMS
        dbprint("Msg: ");
        Transceiver_obj.PrintMsgDatagram(Transceiver_obj.Msg_Datagram);
        dbprint('\n');
#endif
        if (result) {
            if (Transceiver_obj.Ack_Datagram.type & Transceiver::DGT_USER) {
#ifdef DEBUG_PRINT_ACK_DATAGRAMS
                dbprint("Ack: ");
                Transceiver_obj.PrintAckDatagram(Transceiver_obj.Ack_Datagram);
                dbprint('\n');
#endif
                UserLoopAck(Transceiver_obj.Ack_Datagram.message);
            }
        }

#ifdef DEBUG_PRINT_MSG_DATAGRAMS
        else
            dbprintln("Transmission error");
#endif
        //dbprintf("Send time=%lu\n", micros() - start_timer);
    }

    if (Tx_state==MULTIFREQ) {
        if (RunLedEnabled)
            BlinkLed(RUNLED_GPIO, 1000, 20, false);

        if (!result)
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
    }
}

// Return value: 
//  true=MSG datagram sent and ACK datagram of previous datagram received
//  false=MSG datagram sent and ACK datagram of previous datagram not received
bool send(void) {
    bool retval=true;
    static uint16_t Multifreq_number=0; // we'll start frequency hopping *after* sending this datagram
    static uint16_t Session_key=GetRandomInt16(); // random seed used to generate the RF24Channels[] array
    static unsigned long Stat_time=0; // to compute error statistics every second
    static uint16_t Error_counter=0;  // number of transmission errors per second, updated once/second

    if (Tx_state==MULTIFREQ) {
        // clear error statistics every second
        if (millis() >= Stat_time+1000) {
            ErrorCounter=Error_counter; // update the global ErrorCounter once/second
            Error_counter=0;
            Stat_time=millis();
        }
    }

    retval=Transceiver_obj.Send(Msg_type, Msg_message);
    
    if (retval && Transceiver_obj.Ack_Datagram.type & Transceiver::DGT_SERVICE) {
        if (PairingInProgress) {
            if (Transceiver_obj.Ack_Datagram.type & Transceiver::DGT_PAIRING_COMPLETE) {
                dbprintln("Pairing complete, rebooting");
                EndProgram(true); // reset command
            }
        }
        else if ((Transceiver_obj.Ack_Datagram.type & Transceiver::DGT_SYNCHRONIZED) && Multifreq_number==0) {
            // we received the first datagram telling us Rx is synchronized
            Multifreq_number=Transceiver_obj.Ack_Datagram.message[0];
            dbprintf("synchronized after %lu ms\n", millis());
        }
    }

    if (Tx_state==MONOFREQ) {
        if (!PairingInProgress && Multifreq_number && Transceiver_obj.Msg_Datagram.number==Multifreq_number) {
            // we have sent the last datagram of the synchronized sequence : switch to MULTIFREQ
            Tx_state=MULTIFREQ;
            dbprintf("MULTIFREQ after %lu ms, period=%lu µs (%u dg/s)\n", millis(), DGPERIOD, (unsigned int)(1000000/DGPERIOD));
            // assign values to the array of radio channels
            Transceiver_obj.SetSessionKey(Session_key);
            Transceiver_obj.AssignChannels();
#ifdef DEBUG_RANDOM_DISCONNECT
            uint16_t debug_disconnection_seed=GetRandomInt16();
            dbprintf("debug: random disconnection seed=%u\n", debug_disconnection_seed);
            randomSeed(debug_disconnection_seed);
#endif
        }
        else {
            // Send configuration settings to the receiver
            // Tx stays in MONOFREQ until the synchronized sequence is complete or while pairing in progress, sending these DGT_SERVICE datagrams
            // Tx sends these DGT_SERVICE datagrams while Tx_state=MONOFREQ
            static uint16_t device_id=(uint16_t)Settings_obj.GetDeviceId();
            static uint16_t mono_channel=(uint16_t)Settings_obj.GetMonoChannel();
            memset(Msg_message, 0, sizeof(Msg_message));
            Msg_message[0]=device_id;
            Msg_message[1]=mono_channel;
            Msg_message[2]=read_pa_level_switch(PALEVEL0_GPIO, PALEVEL1_GPIO);
            Msg_message[3]=Session_key;
            Msg_type=Transceiver::DGT_SERVICE;

            static micros_t Debug_config_settings_printed_time=0;
            if (millis()>=Debug_config_settings_printed_time+1000) {
                dbprintf("Sending device_id=x%04x mono_channel=x%04x pa_level=%04x session_key=x%04x\n", 
                    Msg_message[0],
                    Msg_message[1],
                    Msg_message[2],
                    Msg_message[3]);
                Debug_config_settings_printed_time=millis();
            }
        }
    }
    if (Tx_state==MULTIFREQ) {
        if (!retval)
            Error_counter++;

        // number of consecutive missing ACK datagrams while MULTIFREQ: we reset the MCU if this counter reaches COM_TRANS_ERRORS
        static uint16_t Reset_counter=0;
        if (retval)
            Reset_counter=0;
        else {
            // reset if we counted COM_TRANS_ERRORS consecutive missing ACK datagrams
            Reset_counter++;
            if (COM_TRANS_ERRORS && Reset_counter==COM_TRANS_ERRORS) {
                // Rx setup() execution time : 205 ms
                // Tx setup() execution time : 165 ms
                dbprintf("Reboot after %d consecutive missed ACK datagrams\n", COM_TRANS_ERRORS);
                EndProgram(true); // reset command
            }
        }
    
        Msg_type=Transceiver::DGT_USER;

        // switch to next radio channel
        Transceiver_obj.SetChannel(Transceiver_obj.Msg_Datagram.number+1);
    }

    // check actions on the Pairing button
    if (Tx_state==MONOFREQ) { // else already paired
        BtnStates btn_state=ReadBtn(PAIRING_GPIO, RUNLED_GPIO, 1500);
        RunLedEnabled=(btn_state==BTN_RELEASED); // if btn_state==BTN_PRESSED then read_button() will control the Led
        if (btn_state==BTN_REACHED_DURATION) {
            // blink led for 3 seconds to tell user that he can release the button
            unsigned long stop_time=millis()+3000;
            while (millis()<stop_time)
                BlinkLed(RUNLED_GPIO, 100, 50, false);
            digitalWrite(RUNLED_GPIO, LOW);

            // reconfigure the transceiver for pairing
            dbprintln("Pairing");
            PairingInProgress=true;
            Transceiver_obj.HotConfig(true, Transceiver::DEF_TXID, Transceiver::DEF_MONOCHAN, Transceiver::DEF_PALEVEL);
        }
    }
#ifdef DEBUG_RANDOM_DISCONNECT
    // reset randomly to simulate connection loss
    if (random(240000)<10) {
        // probability 1/240000 ~ 1 per 20 minutes on average at 100 dg/s = 1 / (240000 / 100 / (60 * 2))
        dbprintf("debug: random reboot after %lu minutes\n", millis()/60000);
        dbprintln("----------------------------------------");
        EndProgram(true); // reset command
    }
#endif
     return retval;
}

// RF output level is hardware-encoded with 2 GPIOs
// 2 bits give 4 possible values: 11=RF24_PA_MIN (0), 10=RF24_PA_LOW (1), 01=RF24_PA_HIGH (2), 00=RF24_PA_MAX (3)
int read_pa_level_switch(uint8_t bit0_gpio, uint8_t bit1_gpio) {
	uint8_t bit0=digitalRead(bit0_gpio)==HIGH?1:0;
	uint8_t bit1=digitalRead(bit1_gpio)==HIGH?1:0;
	uint8_t idx=(2*bit1)+bit0;
	int pa_values[]={RF24_PA_MAX, RF24_PA_HIGH, RF24_PA_LOW, RF24_PA_MIN};
	return pa_values[idx];
}
