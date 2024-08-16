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
#include <rgCsv.h>

class Settings : public rgCsv {
	private:
		/* file param.csv
		# Application parameters
		# Line length 25 chr max
		# Do not change the order
		# of the lines
		TXID,64540
		MONOCHAN,78
		PALEVEL,0
		*/
		const char *PARFILE="param.csv";
				
	public:
		/* inherited methods
		int Open(const char *path_str, int maxlines_int, int maxcells_int, int maxcellen_int);
		void Close(void);
		int Load(void);
		int Save(void);
		int GetIntCell(byte line_int, byte column_int);
		int SetIntCell(byte line_int, byte column_int, int new_value_int);
		const char *GetStrCell(byte line_int, byte column_int);
		int SetStrCell(byte line_int, byte column_int, const char *new_value_str);
		*/

		static const uint8_t NO_BUTTON=255;
		
	    int Init(uint8_t button_gpio, uint8_t led_gpio, 
			uint16_t default_tx_deviceid, 
			uint8_t default_mono_chan, 
			uint8_t default_pa_level);

		int GetDeviceId(void);
		bool SetDeviceId(int value);
		int GetMonoChannel(void);
		bool SetMonoChannel(int value);
		
		// used only by Rx
		int GetPaLevel(void);
		bool SetPaLevel(int value);
};
