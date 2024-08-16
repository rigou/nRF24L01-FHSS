/* rgCsv.cpp
** 2024-08-09
**
** Requirements:
** 	1) partition: Arduino IDE/Tools/Partition scheme/Default 4MB withs spiffs
**  2) filesystem: LittleFS filesystem, already formatted
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

#include "rgStrLib.h"
#include "rgCsv.h"

#define CELLIDX(y,x) (((y)*mMaxcells_int)+(x))

/* Public interface **********************************************************/

/* call this method to allocate resources for this object
	path_str 	absolute path or simple file name
	maxlines_int	max number of data lines in the csv file (not counting blank lines and comments)
	maxcells_int	max number of cells per line
	maxcellen_int	max number of characters per cell (including leading and trailing spaces, if any)
   This method mounts the LittleFS file system if it is not already mounted
   The maximum length of a line in the csv file is (maxcells_int*maxcellen_int)+maxcells_int-1 including
   whitespace and comments ; if a line is too long then Load() will fail
   Return value: 0=success, -1=filesystem not found, -2=file not found, -3=out of memory
*/
int rgCsv::Open(const char *path_str, 
	int maxlines_int, 
	int maxcells_int, 
	int maxcellen_int
) {
	int retval_int=0;
	if (LittleFS.begin(false)) {
		if (mPath_str || mCells)
			Close();
		mPath_str=(char *)calloc(strlen(path_str)+2, sizeof(char)); // +1 for the optional '/', +1 for the final '\0'
		if (mPath_str) {
			byte offset=0;
			if (path_str[0]!='/') {
				mPath_str[0]='/';
				offset=1;
			}
			strncpy(mPath_str+offset, path_str, strlen(path_str));
			if (LittleFS.exists(mPath_str)) {
				mMaxlines_int=maxlines_int;
				mMaxcells_int=maxcells_int;
				mMaxcellen_int=maxcellen_int;

				mCells=(CELL *)calloc(mMaxlines_int*mMaxcells_int, sizeof(CELL));
				if (mCells) {
					for (int idx=0; idx<mMaxlines_int*mMaxcells_int; idx++) {
						mCells[idx].value_int=0;
						mCells[idx].value_str=NULL;
					}
					//Serial.printf("CELL is %d bytes\n", sizeof(CELL));
					//Serial.printf("debug: mCells[] takes %d bytes\n", mMaxlines_int*mMaxcells_int*sizeof(CELL));
				}
				else 
					retval_int=-3; // out of memory
			}
			else
				retval_int=-2; // file not found
		}
		else 
			retval_int=-3; // out of memory
	}
	else
		retval_int=-1; // filesystem not found
	
	return retval_int;
}

/* call this method to free the resources allocated by Open()
   This method leaves the LittleFS file system mounted
*/
void rgCsv::Close(void) {
	if (mPath_str)
		free(mPath_str);
	if (mCells) {
		for (int idx=0; idx<mMaxlines_int*mMaxcells_int; idx++) {
			if (mCells[idx].value_str)
				free(mCells[idx].value_str);
		}
	}
}

/*  Load the whole file in RAM
	Return value : number csv data lines found, or a negative value on error
	-1	file not found
	-2	too many lines
	-3	line too long or missing newline
	-4	MAXCELLS overflow
	-5	MAXCELLENGTH overflow
	-6	out of memory

	zero-length values will be stored as Cell_struct {0, NULL}
*/
int rgCsv::Load(void) {
    int retval_int=0;
	int line_int=0;

	const size_t Line_buffer_size=(mMaxcells_int*mMaxcellen_int)+mMaxcells_int;
	char *linebuff_str=(char *)calloc(Line_buffer_size, sizeof(char));
	char **cells_str_array=(char **)calloc(mMaxcells_int, sizeof(char **));
	
    if (linebuff_str && cells_str_array) {
		File file_obj=LittleFS.open(mPath_str,"r");
		if (file_obj) {
			int eof_int=0;
			while (!eof_int && retval_int==0) {
				switch (read_line(&file_obj, Line_buffer_size, linebuff_str)) {
					case -2:
						eof_int=1;
						break; // EOF

					case -1:
						Serial.printf("debug: max line length is %d\n", Line_buffer_size-1);
						retval_int=-3; // line too long or missing '\n'
						break;

					default:
						if (line_int==mMaxlines_int)
							retval_int=-2; // too many lines
						else {
							// ignore comments and blank lines
							TrimCommentAndWhitespace(linebuff_str);
							if (*linebuff_str) {
								// parse the line
								int result=parse(linebuff_str, cells_str_array, mMaxcells_int, mMaxcellen_int, ',');
								if (result >= 0) {
									// copy cells_str_array to mCells
									for (int column_int=0; column_int<result; column_int++) {
										char *this_cell=cells_str_array[column_int];
										int value_length=strlen(this_cell);
										if (value_length) {
											// test if cell contains only digits
											bool is_integer=true;
											char *ptr=this_cell;
											while (*ptr && is_integer) {
												if (!isdigit((unsigned char)*ptr))
													is_integer=false;
												ptr++;
											}
											if (is_integer) {
												// copy integer value to mCells
												mCells[CELLIDX(line_int, column_int)].value_int=atoi(this_cell);
												mCells[CELLIDX(line_int, column_int)].value_str=NULL;
											}
											else {
												// copy string value to mCells
												char *str=(char *)calloc(value_length+1, sizeof(char));
												if (str) {
													strncpy(str, this_cell, value_length);
													mCells[CELLIDX(line_int, column_int)].value_str=str;
													mCells[CELLIDX(line_int, column_int)].value_int=0;
												}
												else {
													retval_int=-6; // out of memory
													break;
												}
											}
										}
									}
									line_int++;
								}
								else 
									retval_int=result-3;

								// free the contents of cells_str_array but not the array itself
								free_cells(cells_str_array, mMaxcells_int);
							}
						}
						break;
				}
			}
			file_obj.close();
		}
		else
			retval_int=-1; // file not found
	}
	else
		retval_int=-6; // out of memory

	if (linebuff_str)
		free(linebuff_str);
	if (cells_str_array)
		free(cells_str_array);

    if (retval_int == 0) {
        retval_int=line_int;
		mLines_int=line_int;
	}
	else
		mLines_int=0;

    return retval_int;
}

// return value: number of lines written, negative value on error  (do not modify these negative values)
int rgCsv::Save(void) {
	int retval_int=0;
	int line_int=0;
			
	// truncate existing file
	LittleFS.remove(mPath_str);
    File file_obj=LittleFS.open(mPath_str,"w");
    if (file_obj) {
		const size_t Line_buffer_size=(mMaxcells_int*mMaxcellen_int)+mMaxcells_int;
		char *linebuff_str=(char *)calloc(Line_buffer_size, sizeof(char));
		if (linebuff_str) {
			int bufflen_int=0;
			int column_int=0;
			char integer_str[mMaxcellen_int+1]; // "-32767" takes 6 bytes
			while (line_int < mLines_int) {
				memset(linebuff_str, '\0', Line_buffer_size);
				for (column_int=0; column_int<mMaxcells_int; column_int++) {
					const char *str=NULL;
					if (column_int>0)
						strcat(linebuff_str, ",");
					str=GetStrCell(line_int, column_int);
					if (str)
						strcat(linebuff_str, str);
					else {
						snprintf(integer_str, mMaxcellen_int, "%d", GetIntCell(line_int, column_int));
						strcat(linebuff_str, integer_str);
					}
				}
				bufflen_int=strlen(linebuff_str);
				linebuff_str[bufflen_int]='\n';
				linebuff_str[++bufflen_int]='\0';
				int count_int=file_obj.write((const uint8_t *)linebuff_str, bufflen_int);
				if (count_int!=bufflen_int) {
					retval_int=-2; // i/o error writing data
					break;
				}
				line_int++;
			}
			free(linebuff_str);
		}
		file_obj.close();
    }
    else
        retval_int=-1; // i/o error creating file

    if (retval_int == 0)
        retval_int=line_int;
    return retval_int;
}

int rgCsv::GetIntCell(byte line_int, byte column_int) {
	int retval_int=-1;
	if (line_int<mMaxlines_int && column_int<mMaxcells_int)
		retval_int=mCells[CELLIDX(line_int, column_int)].value_int;
	//Serial.printf("GetIntCell(%d, %d) returns %d\n", line_int, column_int, retval_int);
	return retval_int;
}

// if the cell contains an existing string value then this value will be NULLed
int rgCsv::SetIntCell(byte line_int, byte column_int, int new_value_int) {
	int retval_int=0;
	if (line_int<mMaxlines_int && column_int<mMaxcells_int) {
		retval_int=SetStrCell(line_int, column_int, NULL);
		mCells[CELLIDX(line_int, column_int)].value_int=new_value_int;
	}
	else
		retval_int=-1; // index out of range
	return retval_int;
}

const char *rgCsv::GetStrCell(byte line_int, byte column_int) {
	const char *retval_str=NULL;
	if (line_int<mMaxlines_int && column_int<mMaxcells_int)
		retval_str=mCells[CELLIDX(line_int, column_int)].value_str;
	//Serial.printf("GetStrCell(%d, %d) returns \"%s\"\n", line_int, column_int, retval_str);
	return retval_str;
}

// if the cell contains an existing integer value then this value will be zeroed
// a zero-length new_value_str or a NULL value will be stored as a Cell_struct {0, NULL}
int rgCsv::SetStrCell(byte line_int, byte column_int, const char *new_value_str) {
	int retval_int=0;
		
	if (line_int<mMaxlines_int && column_int<mMaxcells_int) {
		int new_length=0;
		if (new_value_str)
			new_length=strlen(new_value_str);

		char *str=mCells[CELLIDX(line_int, column_int)].value_str;
		if (str)
			free(str);
		if (new_length) {
			str=(char *)calloc(new_length+1, sizeof(char));
			if (str)
				strncpy(str, new_value_str, new_length);
			else
				retval_int=-2; // out of memory
		}
		else
			str=NULL;
		if (retval_int==0) {
			mCells[CELLIDX(line_int, column_int)].value_str=str;
			mCells[CELLIDX(line_int, column_int)].value_int=0;
		}
	}
	else
		retval_int=-1; // index out of range
	return retval_int;
}


/* Private implementation ****************************************************/

/* Extract the CSV cells from given line into given array of strings
 * line_str is a string in csv format, containing no linebreaks
 * max_cells maximum number of cells in the line
 * cell_size maximum length of a cell, not counting the final '\0'
 * cell_separator is the character between cells
 ** Return value: number of values stored in given cells_str_array_out, or negative on error:
 *	-1 max cells overflow
 *	-2 cell size overflow
 *  -3 out of memory
 */			
int rgCsv::parse(const char *line_str, char **cells_str_array_out, const int max_cells, const int cell_size, const char cell_separator) {
	int retval=0;
	for (int idx=0; idx<max_cells; idx++)
		cells_str_array_out[idx]=NULL;
	if (*line_str) {
		int cell_count=0;
		int char_count=0;
		const char *line_ptr=line_str;
		char cell_buff[cell_size+1];
		while (1) {
			if (*line_ptr==cell_separator || *line_ptr=='\0') {
				// acquire a cell
				if (cell_count<max_cells) {
					cell_buff[char_count]='\0';
					TrimWhitespace(cell_buff);
					char_count=strlen(cell_buff);
					char *str=(char *)calloc(char_count+1, sizeof(char));
					if (str) {
						strncpy(str, cell_buff, char_count+1);
						cells_str_array_out[cell_count]=str;
						cell_count++;
						if (*line_ptr=='\0') {
							retval=cell_count;
							break;
						}
						char_count=0;
					}
					else {
						retval=-3; // out of memory
						break;
					}
				}
				else {
					retval=-1; // max cells overflow
					break;
				}
			}
			else {
				// acquire a character
				if (char_count<cell_size)
					cell_buff[char_count++]=*line_ptr;
				else {
					retval=-2; // cell size overflow
					break;
				}
			}
			line_ptr++;
		}
	}
	return retval;
}

// free the contents of cells_str_array_out but not the array itself
// parse() has allocated the memory of theses cells
void rgCsv::free_cells(char **cells_str_array_out, const int max_cells) {
	for (int idx=0; idx<max_cells; idx++) {
		if (cells_str_array_out[idx]) {
			free(cells_str_array_out[idx]);
			cells_str_array_out[idx]=NULL;
		}
	}

}

// Read a line in given file and store it into given buffer
// buffer must have room to store the terminating '\0' char
// buffer_size_int is the total length, including the terminating '\0'
// any '\r' is ignored, '\n' is not copied to the buffer
// Return values:
// * length of the line in given buffer (may be 0) if the line was read correctly
// * or -1 and strlen(buffer)>0 if the line was too long to fit in the buffer or if we read
//     the last line of the file and it was not terminated by '\n'
//     whatever data was read is available in the buffer
// * or -2 and strlen(buffer)=0 on EOF 
int rgCsv::read_line(File *file_obj, int buffer_size_int, char *buffer_out_str) {
	int retval_int=-1;
	int char_counter_int=0;
	char *buffer_ptr=buffer_out_str;
	memset(buffer_out_str, '\0', buffer_size_int);
	while (file_obj->available()) {
		if (char_counter_int < buffer_size_int) {
			char this_char=file_obj->read();
			if (this_char!='\r') {
				if (this_char=='\n') {
					retval_int=char_counter_int;
					break;
				}
				else {
					*buffer_ptr++=this_char;
					char_counter_int++;
				}
			}
		}
		else {
			retval_int=-1; // '\n' not found before end of buffer
			break;
		}
	}
	if (retval_int==-1 && char_counter_int==0)
		retval_int=-2; // EOF
	//Serial.printf("debug: %s\t(%d)\n", buffer_out_str, retval_int);
	return retval_int;
}
