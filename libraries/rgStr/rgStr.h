/* rgStr.h - Library
** 2022-11-07 derived from arduinotx project
*/
#pragma once

#include <Arduino.h>

#define STRLIB_NAME	"rgStr" // spaces not permitted
#define STRLIB_VERSION	"v1.0.0"

short Isblank(const char *line_str);
char *TrimCommentAndWhitespace(char *out_line_str);
char *TrimWhitespace(char *out_line_str);
char *TimeString(unsigned long seconds_lng, char *out_buffer_str);
unsigned long Hex2Dec(char *hex_str, short length_byt);
char *BinStr(char *buffer_str, int nbits_int, unsigned int num_int);
