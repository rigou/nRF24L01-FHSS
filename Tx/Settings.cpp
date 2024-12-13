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
#include <rgBtn.h>

// 0=debug off, 1=output to serial, 2=output to serial and optionally bluetooth with dbtprintln()
#define DEBUG_ON 1
// 0=trace off, 1=output to serial, 2=output to serial and optionally bluetooth with trbtprintln()
#define TRACE_ON 0
#include <rgDebug.h>

/*****************************************
* Application parameters file param.csv
*****************************************/
enum class Paramid {
	TXID,
	RXID,
	MONOCHAN,
	PALEVEL
};
rgCsv ParamCsv_obj;
// we use these macros to simplify access to the mCells array of ParamCsv_obj
#define PARAMGETSTR(x)		GetStrCell((byte)Paramid::x, 0)
#define PARAMSETSTR(x,v)	SetStrCell((byte)Paramid::x, 0, (v))
#define PARAMGETINT(x)		GetIntCell((byte)Paramid::x, 1)
#define PARAMSETINT(x,v)	SetIntCell((byte)Paramid::x, 1, (v))

// Mount the filesystem and read current settings from PARFILE
// PARFILE will be created if not found
// PARFILE will be overwritten with default values if the given button is pressed long enough
// return value: 0=ok, not 0 on error :
//	1	settings file creation failed
//	2	settings file not found, or empty, or read failed
//	3	SPIFFS filesystem mount failed
//  4	out of memory
//  5   file system corrupted
int Settings::Init(
	uint16_t default_tx_deviceid,
	uint16_t default_rx_deviceid, 
	uint8_t default_mono_chan, 
	uint8_t default_pa_level
) {
	trprintf("*** %s %s() begin\n", __FILE_NAME__, __FUNCTION__);
	int retval=0;
	
	// Mount the file system and allocate memory for the file
	bool create_settings_file=false;
	int nrecords=0;
	switch (Allocate(PARFILE, PAR_MAXLINES, PAR_MAXCELLS, PAR_MAXCELLEN, false)) {
	case 0:
		/*  Load the whole file in RAM
			Load() return value : number of key/value pairs found, or a negative value on error
			-1	file not found
			-2	key empty or too long
			-3	key/value separator not found
			-4	value too long
			-5	too many lines
			-6	line too long or missing newline
		*/
		nrecords=Load(); // returns the number of csv data lines found, or a negative value on error 
		if (nrecords>0) { 
			dbprintf("%s contains %d records : ", PARFILE, nrecords);
			dbprintf("TxId 0x%06x (%d), RxId 0x%06x (%d), Chan 0x%02x (%d), Pa_level %d\n", 
				GetTxDeviceId(), GetTxDeviceId(), GetRxDeviceId(), GetRxDeviceId(), GetMonoChannel(), GetMonoChannel(), GetPaLevel());
			retval=0;
		}
		else if (nrecords==0) {
			dbprintln("Init: settings file is empty");
			create_settings_file=true;
		}
		else if (nrecords==-1) {
			dbprintln("Init: settings file not found");
			create_settings_file=true;
		}
		else {
			dbprintf("Init: error %d accessing settings file\n", nrecords);
			retval=2;
		}
		break;

	case -1:
		dbprintln("Init: file system corrupted");
		retval=5;
		break;

	case -2:
		// file system ok but settings file not found
		create_settings_file=true; 
		break;

	case -3:
		dbprintln("Init: out of memory error");
		retval=4;
		break;
	}

	if (create_settings_file) {
		Release();
		retval=create_settings(default_tx_deviceid, default_rx_deviceid, default_mono_chan, default_pa_level);
	}
	
	trprintf("*** %s %s() returns %d\n", __FILE_NAME__, __FUNCTION__, retval);
    return retval;
}

int Settings::GetTxDeviceId(void) {
	return PARAMGETINT(TXID);
}

bool Settings::SetTxDeviceId(int value) {
	bool retval=true;
	if (PARAMSETINT(TXID, value)<0)  {
		dbprintln("TXID write error");
		retval=false;
	}
	return retval;
}

int Settings::GetRxDeviceId(void) {
	return PARAMGETINT(RXID);
}

bool Settings::SetRxDeviceId(int value) {
	bool retval=true;
	if (PARAMSETINT(RXID, value)<0)  {
		dbprintln("RXID write error");
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

// return value: 0=ok, not 0 on error :
//	1	settings file creation failed
//	2	settings read failed
//  4	out of memory
int Settings::create_settings(
	uint16_t tx_deviceid,
	uint16_t rx_deviceid,
	uint8_t mono_chan,
	uint8_t pa_level
) {
	int retval=0;

	// Mount the file system and allocate memory for the file
	if (Allocate(PARFILE, PAR_MAXLINES, PAR_MAXCELLS, PAR_MAXCELLEN, true)==0) {
		dbprintln("create_settings: creating settings file...");

		// This value uniquely identifies the transmitter
		SetTxDeviceId(tx_deviceid);

		// This value uniquely identifies the receiver
		SetRxDeviceId(rx_deviceid);

		// we use this radio channel when transmitting in single frequency
		// channel values: 0-125, actual frequency=2400+channel, eg 2483 MHz for channel 83
		// your local laws may not allow the full frequency range (authorized frequencies in France: 2400-2483.5 MHz)
		SetMonoChannel(mono_chan);

		// Rx uses this default Amplifier (PA) level and Low Noise Amplifier (LNA) state at boot
		// Later, Tx receives a new value from Tx and stores it here
		// Tx uses the value returned by read_pa_level_switch() and does not access this key
		SetPaLevel(pa_level);

		int nrecords=Save(4); // returns the number of lines written, negative value on error, or a negative value on error 
		if (nrecords>=0) {
			dbprintf("create_settings: settings file created with %d records\n", nrecords);
			retval=0;
		}
		else {
			dbprintln("create_settings: error %d creating settings file");
			retval=1;
		}
	}
	else {
		dbprintln("create_settings: Allocate() error");
		retval=1; // Allocate() failed
	}
	//dbprintf("create_settings() returns %d\n", retval);
	return retval;
}
