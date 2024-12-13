/* rgStr.cpp - Library
** 2022-11-07 derived from arduinotx project
** 2024-07-21 some renaming, better implementation of TrimWhitespace(), new TrimCommentAndWhitespace()
*/

#include "rgStr.h"

static short ishexdigit(char a_chr);

// Test if given null-terminated string is empty or contains only white-space characters
// return value: 1=string is empty or blank, 0=string contains non-blank characters
short Isblank(const char *line_str) {
	short retval_byt = 1;
	const char *c_ptr = line_str;
	while (*c_ptr && retval_byt) {
		if (isSpace(*c_ptr))
			c_ptr++;
		else
			retval_byt = 0;
	}
	return retval_byt;
}

char *TrimCommentAndWhitespace(char *line_out) {
	char *ptr =strchr(line_out, '#');
	if (ptr)
		*ptr='\0';
	return TrimWhitespace(line_out);
}

char *TrimWhitespace(char *line_out) {
	char *ptr = line_out;
	while (isspace(*ptr))
		ptr++;
	if (*ptr) {
		char *last_chr = ptr + strlen(ptr) - 1;
		while (last_chr > ptr && isspace(*last_chr))
			last_chr--;
		*(last_chr+1) = 0;
	}
	if (ptr!=line_out)
		memcpy(line_out, ptr, strlen(ptr)+1);
	return line_out;
}


// Format given time in seconds in given user-allocated buffer (11 chars)
// Returns hhhh:mm:ss
char *TimeString(unsigned long seconds_lng, char *out_buffer_str) {
	unsigned long m = seconds_lng / 60;
	unsigned long h = seconds_lng / 3600;
	m = m - (h * 60);
	sprintf(out_buffer_str, "%02ld:%02ld:%02ld", h, m, seconds_lng - (m * 60) - (h * 3600));
	return out_buffer_str;
}

// convert hex_str to long int
// stops conversion when a non-hex char is found
unsigned long Hex2Dec(char *hex_str, short length_byt) {
	unsigned long retval_lng = 0;
	for (short idx_byt=0; idx_byt<length_byt; idx_byt++) {
		char x_chr = toupper(hex_str[idx_byt]);
		if (ishexdigit(x_chr))
			retval_lng += (x_chr - ( x_chr <= '9' ? '0':'7')) * (1<<(4*(length_byt-1-idx_byt)));
		else
			break;
	}
	return retval_lng;
}

// return the LSB binary string representation of given integer value, with given number of bits, into given caller-allocated buffer
char *BinStr(char *buffer_str, int nbits_int, unsigned int num_int) {
	unsigned int idx_int=0;
	unsigned int mask_int=1;
    while (idx_int < nbits_int) {
    	buffer_str[idx_int]=(mask_int & num_int)?'1':'0';
        mask_int<<=1;
        idx_int++;
    }
	buffer_str[idx_int]='\0';
    return buffer_str;
}

static short ishexdigit(char a_chr) {
	return (a_chr >= '0' && a_chr <= '9') || (a_chr >= 'A' && a_chr <= 'F') || (a_chr >= 'a' && a_chr <= 'f');;
}
