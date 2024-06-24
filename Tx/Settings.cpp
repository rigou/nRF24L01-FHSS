/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#include "Settings.h"
#include "Common.h"
#include <rgButton.h>

// Mount the filesystem and read current settings from PARFILE
// PARFILE will be created if not found
// PARFILE will be overwritten with default values if the given button is pressed long enough
// button_gpio and led_gpio : press the given button during boot and the led turns on,
//   then when the led starts flashing, release the button
//   If button_gpio==255 then ignore button
// return value: 0=ok, not 0 on error :
//	1	settings file creation failed
//	2	settings file read failed
//	3	SPIFFS filesystem mount failed
int Settings::Init(
	byte button_gpio,
	byte led_gpio, 
	uint16_t default_tx_deviceid, 
	byte default_mono_chan, 
	byte default_pa_level
) {
	const int BUTTON_HOLD=3000; // ms, hold the button long enough to create a new file with default values
	int retval=0;

	// Mount the file system and allocate memory for the file
	// format_fs_if_failed = 1
	// max_len_path = 16
	// max_len_key  = 8
	// max_len_value = 8 // enough for storing the hexadecimal representation of a 32 bit unsigned integer
	// max_records = 3   // "TXID", "MONOCHAN", "PALEVEL" (PALEVEL is used only by Rx)
	int init_result=rgParam::Init(PARFILE, 1, 16, 8, 8, 3);
	if (init_result==0) {

		// Hold the Pairing button during boot to delete the current settings file
		if (button_gpio!=NO_BUTTON) {
			BtnStates btn_state=ReadBtnBlock(button_gpio, led_gpio, BUTTON_HOLD); // ms
			if (btn_state==BTN_REACHED_DURATION) {
				if (Remove())
					Serial.println("Init: error deleting settings file"); // continue anyway
			}
		}
		/*  Load the whole file in RAM
			Return value : number of key/value pairs found, or a negative value on error
			-1	file not found
			-2	key empty or too long
			-3	key/value separator not found
			-4	value too long
			-5	too many lines
			-6	line too long or missing newline
		*/
		int nrecords=Load(); // returns the number of key/value pairs found, or a negative value on error 
		if (nrecords == -1) {
			// file not found : create the file with default values
			Serial.println("Init: creating settings file");

			// This string uniquely identifies the device
			SetDeviceId(default_tx_deviceid);

			// we use this radio channel when transmitting in single frequency
			// channel values: 0-125, actual frequency=2400+channel, eg 2483 MHz for channel 83
			// your local laws may not allow the full frequency range (authorized frequencies in France: 2400-2483.5 MHz)
			SetMonoChannel(default_mono_chan);

			// Rx uses this default Amplifier (PA) level and Low Noise Amplifier (LNA) state at boot
			// Later, Tx receives a new value from Tx and stores it here
			// Tx uses the value returned by read_pa_level_switch() and does not access this key
			SetPaLevel(default_pa_level);

			nrecords=Save(); // returns the number of key/value pairs written, or a negative value on error 
			if (nrecords>=0) {
				Serial.println("Init: settings file created");
				retval=0;
			}
			else {
				Serial.printf("Init: error %d creating settings file\n", nrecords);
				retval=1;
			}
		}
		if (nrecords>=0) { 
			Serial.printf("Init: settings file contains %d records\n", nrecords);
			retval=0;
		}
		else {
			Serial.printf("Init: error %d accessing settings file\n", nrecords);
			retval=2;
		}
		SerialDump(PARFILE); // $$DEBUG

	}
	else {
		Serial.println("Init: failed mounting SPIFFS filesystem");
		retval=3;
	}

    return retval;
}

int Settings::GetDeviceId(void) {
	int retval=0;
	unsigned int value=0;
	if (GetKeyInt("TXID", &value)>=0)
		retval=value;
	else {
		Serial.println("TXID not found");
		retval=-1;
	}
	return retval;
}

bool Settings::SetDeviceId(int value) {
	bool retval=true;
	if (SetKeyInt("TXID", value)<0)  {
		Serial.println("TXID write error");
		retval=false;
	}
	return retval;
}

int Settings::GetMonoChannel() {
	int retval=0;
	unsigned int value=0;
	if (GetKeyInt("MONOCHAN", &value)>=0)
		retval=value;
	else {
		Serial.println("MONOCHAN not found");
		retval=-1;
	}
	return retval;
}

bool Settings::SetMonoChannel(int value) {
	bool retval=true;
	if (SetKeyInt("MONOCHAN", value)<0)  {
		Serial.println("MONOCHAN write error");
		retval=false;
	}
	return retval;
}

int Settings::GetPaLevel() {
	int retval=0;
	unsigned int value=0;
	if (GetKeyInt("PALEVEL", &value)>=0)
		retval=value;
	else {
		Serial.println("PALEVEL not found");
		retval=-1;
	}
	return retval;
}

bool Settings::SetPaLevel(int value) {
	bool retval=true;
	if (SetKeyInt("PALEVEL", value)<0)  {
		Serial.println("PALEVEL write error");
		retval=false;
	}
	return retval;
}
