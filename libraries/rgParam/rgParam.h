/* This program is published under the GNU General Public License. 
 * This program is free software and you can redistribute it and/or modify it under the terms
 * of the GNU General  Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
 * See the GNU General Public License for more details : https://www.gnu.org/licenses/ *GPL 
 *
 * Installation, usage : https://github.com/rigou/nRF24L01-FHSS/
*/

#pragma once

/* LittleFS.h creates the external reference 'LittleFS' to the little filesystem class, through which all file i/o will be performed
	$ find ~/.arduino15 -type f -name 'LittleFS.h'
		~/.arduino15/packages/esp32/hardware/esp32/2.0.7/libraries/LittleFS/src/LittleFS.h
		~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/LittleFS/src/LittleFS.h
	$ find ~/.arduino15 -type f -name 'FS.h'
		/home/rigou/.arduino15/packages/esp32/hardware/esp32/2.0.7/libraries/FS/src/FS.h
		/home/rigou/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/cores/esp8266/FS.h

*/
#include "LittleFS.h"

#define PARAMLIB_NAME	"rgParam" // spaces not permitted
#define PARAMLIB_VERSION	"v2.4.0"

// set trace levels: 0=no trace, 1=normal, 2=verbose
#define RGPARAM_TRACE	0

#define RGPARAM_BUFFER_LEN	(MaxLenKey+MaxLenValue+3)

class rgParam {
	public:
		static const unsigned int UINT32_MAXVALUE=4294967295; // 0xffffffff

		// values set by Init()
		int MaxLenPath=0;
		int MaxLenKey=0;
		int MaxLenValue=0;
		int MaxRecords=0;

		int Init(const char *path_str=NULL, int format_fs_if_failed_int=1, int max_len_path=16, int max_len_key=8, int max_len_value=12, int max_records=16);
		void End(void);
		int Load(const char *path_str=NULL); // if path_str == NULL then use mPath_str else copy path_str into mPath_str
		int Save(const char *path_str=NULL); // if path_str == NULL then use mPath_str else copy path_str into mPath_str
		void Clear(void); // clear all keys and values in memory arrays only
		int Remove(const char *path_str=NULL); // if path_str == NULL then use mPath_str else copy path_str into mPath_str
		int GetKeyStr(const char *key_str, char *value_out_str); // store value of given key into given user-allocated value_out_str
		int SetKeyStr(const char *key_str, const char *value_str); // copy given key and given value in memory arrays
		int GetKeyInt(const char *key_str, unsigned int *value_out_int);
		int SetKeyInt(const char *key_str, unsigned int value_int);
		int DelKey(const char *key_str); // clear given key and its corresponding value in memory arrays
		int Dump(const char *path_str, int buffer_size_int, char *buffer_out_str); // copy the contents of the file into user-allocated buffer
		void SerialDump(const char *path_str=NULL); // print the raw contents of the file on Serial

	private:
		static const char FIELDSEP='\t';
		static const char LINESEP='\n';

		char *mBuffer_str=NULL; // general purpose buffer, its size is defined by the RGPARAM_BUFFER_LEN macro
		char *mPath_str=NULL; // must start with '/' (absolute path)
		char **mKeys_str=NULL;
		char **mValues_str=NULL;

		int read_line(File *file_obj, int buffer_size_int, char *buffer_out_str);
		int parse_line(char *line_str_out, char **key_ptr_out, char **value_ptr_out);

#if RGPARAM_TRACE >= 1
		static const int TRACELEN=100;
		char Trace_str[TRACELEN];
		void Trace(const char *text_str=NULL);
#endif


};
