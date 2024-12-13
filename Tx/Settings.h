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
		TXID,256
		RXID,512
		MONOCHAN,64
		PALEVEL,0
		*/
		static const int PAR_MAXLINES=4;
		static const int PAR_MAXCELLS=2;
		static const int PAR_MAXCELLEN=12;
		
		int create_settings(uint16_t tx_deviceid, uint16_t rx_deviceid, uint8_t mono_chan, uint8_t pa_level);
		
	public:
		const char *PARFILE="/param.csv";

		/* inherited methods
		int Open(const char *path_str, int maxlines_int, int maxcells_int, int maxcellen_int);
		void Close(void);
		int Load(void);
		int Save(void);
		int16_t GetIntCell(byte line_int, byte column_int);
		int SetIntCell(byte line_int, byte column_int, int16_t new_value_int);
		const char *GetStrCell(byte line_int, byte column_int);
		int SetStrCell(byte line_int, byte column_int, const char *new_value_str);
		*/
		
	    int Init(
			uint16_t default_tx_deviceid,
			uint16_t default_rx_deviceid,
			uint8_t default_mono_chan,
			uint8_t default_pa_level
		);
		int GetTxDeviceId(void);
		bool SetTxDeviceId(int value);
		int GetRxDeviceId(void);
		bool SetRxDeviceId(int value);
		int GetMonoChannel(void);
		bool SetMonoChannel(int value);
		
		// used only by Rx
		int GetPaLevel(void);
		bool SetPaLevel(int value);
};
