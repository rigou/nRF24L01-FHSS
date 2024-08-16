/* rgCsv.h
** 2024-08-11
*/

/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
 * 
 * This class is derived from original code found at https://en.wikipedia.org/wiki/Xorshift#xorwow
*/

#pragma once
#include <LittleFS.h>

#define CSVLIB_NAME	"rgCsv" // spaces not permitted
#define CSVLIB_VERSION	"v1.2.2"

/*******************************************
* How to emulate library rgParam with rgCsv
********************************************
Example: file param.csv contains 3 parameters
	# Application parameters
	# Line length 25 chr max
	# Do not change the order
	# of the lines
	TXID,64540
	MONOCHAN,78
	PALEVEL,0

Declare this in your sketch:
	#include <rgCsv.h>
	enum class Paramid {
		TXID,
		MONOCHAN,
		PALEVEL
	};
	rgCsv ParamCsv_obj;

	// you can use these macros to simplify access to the mCells array of ParamCsv_obj
	#define PARAMGETINT(x)		GetIntCell(byte(Paramid::x),1)
	#define PARAMSETINT(x,v)	SetIntCell((byte)Paramid::x, 1, (v))
*/

class rgCsv {
	private:
		char *mPath_str=NULL;
		int mMaxlines_int=0;
		int mMaxcells_int=0;
		int mMaxcellen_int=0;
		
		// either one of the two members of this structure contains a value, but not both
		struct Cell_struct {
			int16_t value_int;
			char *value_str;
		};
		typedef struct Cell_struct CELL;
		
		CELL *mCells; // an array of CELL structures
		int mLines_int=0; // number of lines stored in mCells[] by Load()

		int parse(const char *line_str, char **cells_str_array_out, const int max_cells, const int cell_size, const char cell_separator);
		void free_cells(char **cells_str_array_out, const int max_cells);
		int read_line(File *file_obj, int buffer_size_int, char *buffer_out_str);

	public:
		int Open(const char *path_str, int maxlines_int, int maxcells_int, int maxcellen_int);
		void Close(void);
		int Load(void);
		int Save(void);
		int GetIntCell(byte line_int, byte column_int);
		int SetIntCell(byte line_int, byte column_int, int new_value_int);
		const char *GetStrCell(byte line_int, byte column_int);
		int SetStrCell(byte line_int, byte column_int, const char *new_value_str);
};
