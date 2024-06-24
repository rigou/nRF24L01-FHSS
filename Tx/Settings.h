/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#pragma once
#include <rgParam.h>

class Settings : public rgParam {
	private:
		// Application settings
		const char *PARFILE="/RF24.PAR";
				
	public:
		// inherited methods
		//~ int Init(void);
		//~ int Load(void);
		//~ int Save(void);
		//~ void Clear(void); // reset all settings to their default values in memory only
		//~ int Dump(int buffer_size_int, char *buffer_out_str); // copy the raw contents of the file into buffer
		//~ void SerialDump(void);

		static const byte NO_BUTTON=255;
		
	    int Init(byte button_gpio, byte led_gpio, 
		uint16_t default_tx_deviceid, 
		byte default_mono_chan, 
		byte default_pa_level); // overloaded

		int GetDeviceId(void);
		bool SetDeviceId(int value);
		int GetMonoChannel(void);
		bool SetMonoChannel(int value);
		
		// used only by Rx
		int GetPaLevel(void);
		bool SetPaLevel(int value);
};
