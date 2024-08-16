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

#include "Settings.h"
#include "Common.h"
#include <rgButton.h>

/*****************************************
* Application parameters file param.csv
*****************************************/
enum class Paramid {
	TXID,
	MONOCHAN,
	PALEVEL
};
rgCsv ParamCsv_obj;
// we use these macros to simplify access to the mCells array of ParamCsv_obj
#define PARAMGETINT(x)		GetIntCell(byte(Paramid::x),1)
#define PARAMSETINT(x,v)	SetIntCell((byte)Paramid::x, 1, (v))

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
	uint8_t button_gpio,
	uint8_t led_gpio, 
	uint16_t default_tx_deviceid, 
	uint8_t default_mono_chan, 
	uint8_t default_pa_level
) {
	const int BUTTON_HOLD=3000; // ms, hold the button long enough to create a new file with default values
	int retval=0;

	// Mount the file system and allocate memory for the file
	// 3 lines, 2 columns, 12 chr / cell
	int open_result=Open(PARFILE, 3, 2, 12);
	if (open_result==0) {

		// Hold the Pairing button during boot to delete the current settings file
		if (button_gpio!=NO_BUTTON) {
			BtnStates btn_state=ReadBtnBlock(button_gpio, led_gpio, BUTTON_HOLD); // ms
			if (btn_state==BTN_REACHED_DURATION) {
				if (!LittleFS.remove(PARFILE))
					dbprintln("Init: error deleting settings file"); // continue anyway
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
		int nrecords=Load(); // returns the number of csv data lines found, or a negative value on error 
		if (nrecords == -1) {
			// file not found : create the file with default values
			dbprintln("Init: creating settings file");

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

			nrecords=Save(); // returns the number of lines written, negative value on error, or a negative value on error 
			if (nrecords>=0) {
				dbprintln("Init: settings file created");
				retval=0;
			}
			else {
				dbprintf("Init: error %d creating settings file\n", nrecords);
				retval=1;
			}
		}
		if (nrecords>0) { 
			dbprintf("Init: settings file contains %d records\n", nrecords);
			retval=0;
		}
		else {
			dbprintf("Init: error %d accessing settings file\n", nrecords);
			retval=2;
		}
	}
	else {
		dbprintf("Init: file %s or filesystem not found\n", PARFILE);
		retval=3;
	}
    return retval;
}

int Settings::GetDeviceId(void) {
	return PARAMGETINT(TXID);
}

bool Settings::SetDeviceId(int value) {
	bool retval=true;
	if (PARAMSETINT(TXID, value)<0)  {
		dbprintln("TXID write error");
		retval=false;
	}
	return retval;
}

int Settings::GetMonoChannel() {
	return PARAMGETINT(MONOCHAN);
}

bool Settings::SetMonoChannel(int value) {
	bool retval=true;
	if (PARAMSETINT(MONOCHAN, value)<0)  {
		dbprintln("MONOCHAN write error");
		retval=false;
	}
	return retval;
}

int Settings::GetPaLevel() {
	return PARAMGETINT(PALEVEL);
}

bool Settings::SetPaLevel(int value) {
	bool retval=true;
	if (PARAMSETINT(PALEVEL, value)<0)  {
		dbprintln("PALEVEL write error");
		retval=false;
	}
	return retval;
}
