
// crcpp.rl - a simple CRC-32 macro inserter for C++ or other CPP-preprocessed languages

// Future improvements:
// Support string concatenation with quote-quote (right now only backslashed newlines are supported).
// Handle too-long-line error (but for now the line length is long enough there will be no problem in reasonable circumstances).
// Handle user typos by relaxing the syntax when a case comes up that calls for it.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#define RENAME(x,y) MoveFileExA(x, y, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)

// avoid conflict with eof() by defining __STDC__
#define __STDC__ 1
#include <io.h>
#define MKTEMP _mktemp
#else
#define RENAME rename
#define MKTEMP mktemp
#define BOOL int
#define TRUE (1)
#define FALSE (0)
#endif

#define MAX_LINE_LEN 256*1024-2
#define READ_BUF_SIZE 2*MAX_LINE_LEN

FILE *outputFile;
char argument_buffer[MAX_LINE_LEN+1];
char unquoted_argument_buffer[MAX_LINE_LEN+1];
char *argument_start_mark;
unsigned long last_argument_crc;
BOOL last_argument_hex_present;
unsigned long last_argument_hex;

char line_buffer[MAX_LINE_LEN+2];
int line_buffer_len;

BOOL writeOutValues;
int numMissingValues;

static void parseFile(FILE *inputFile);
static void unquote(char *unquotedString, const char *quotedString);

// Ragel data variables
size_t cs;
char *p;
char *pe;
char *eof;

int act;
char *ts;
char *te;
int stack[128], top;







static const char _ma_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 2, 0, 1, 2, 3, 
	8
};

static const char _ma_key_offsets[] = {
	0, 1, 2, 6, 8, 10, 15, 19, 
	20, 26, 36, 40, 40, 42
};

static const char _ma_trans_keys[] = {
	67, 40, 32, 34, 9, 13, 34, 92, 
	34, 92, 32, 41, 44, 9, 13, 32, 
	48, 9, 13, 120, 48, 57, 65, 70, 
	97, 102, 32, 41, 9, 13, 48, 57, 
	65, 70, 97, 102, 32, 41, 9, 13, 
	10, 67, 82, 0
};

static const char _ma_single_lengths[] = {
	1, 1, 2, 2, 2, 3, 2, 1, 
	0, 2, 2, 0, 2, 1
};

static const char _ma_range_lengths[] = {
	0, 0, 1, 0, 0, 1, 1, 0, 
	3, 4, 1, 0, 0, 0
};

static const char _ma_index_offsets[] = {
	0, 2, 4, 8, 11, 14, 19, 23, 
	25, 29, 36, 40, 41, 44
};

static const char _ma_trans_targs[] = {
	1, 12, 2, 12, 2, 3, 2, 12, 
	5, 11, 4, 5, 11, 4, 5, 12, 
	6, 5, 12, 6, 7, 6, 12, 8, 
	12, 9, 9, 9, 12, 10, 12, 10, 
	9, 9, 9, 12, 10, 12, 10, 12, 
	4, 12, 13, 12, 0, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 0
};

static const char _ma_trans_actions[] = {
	0, 25, 0, 25, 0, 0, 0, 25, 
	27, 1, 1, 3, 0, 0, 0, 17, 
	0, 0, 25, 0, 5, 0, 25, 0, 
	25, 0, 0, 0, 25, 7, 30, 7, 
	0, 0, 0, 25, 0, 17, 0, 25, 
	0, 19, 15, 21, 0, 23, 25, 25, 
	25, 25, 25, 25, 25, 25, 25, 25, 
	25, 25, 23, 0
};

static const char _ma_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 11, 0
};

static const char _ma_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 13, 0
};

static const char _ma_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 9, 0
};

static const char _ma_eof_trans[] = {
	58, 58, 58, 58, 58, 58, 58, 58, 
	58, 58, 58, 58, 0, 59
};

static const int ma_start = 12;
static const int ma_first_final = 12;
static const int ma_error = -1;

static const int ma_en_main = 12;




int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("Usage: %s <file_with_crc_macros>\n", argv[0]);
	} else {
		// use stdin if "-" supplied as filename
		char *fileName = argv[1];
		FILE *inputFile = fopen(fileName, "r");
		char tempFileName[FILENAME_MAX+8];

		if (inputFile) {
			

	{
	cs = ma_start;
	ts = 0;
	te = 0;
	act = 0;
	}


			writeOutValues = FALSE;
			parseFile(inputFile);

			if (numMissingValues) {
				fseek(inputFile, 0, SEEK_SET);

				// create a temp file for in-place editing
				strcpy(tempFileName, fileName);
				strcat(tempFileName, ".XXXXXX");
				MKTEMP(tempFileName);

				outputFile = fopen(tempFileName, "w");
				if (outputFile) {
					writeOutValues = TRUE;
					parseFile(inputFile);
					fclose(inputFile);
					fclose(outputFile);

					// now put the temp file in place of the input file
					RENAME(tempFileName, fileName);
				} else {
					fclose(inputFile);
				}
			}
		} else {
			fprintf(stderr, "Error: cannot open file %s", fileName);
		}
	}

	return 0;
}


// Uses input strategy from section 5.9 of Ragel user guide -
// reverse-scan input buffer for a known separator and use that to chunk
void parseFile(FILE *inputFile)
{
	static char buf[READ_BUF_SIZE];
	char *bufReadStart = buf;

	// read file in chunks
	for (;;) {
		char *dataEnd;
		size_t maxReadSize = READ_BUF_SIZE - (size_t)(bufReadStart - buf);
		size_t dataLength = fread(buf, 1, maxReadSize, inputFile);
		
		if (dataLength == 0 && bufReadStart == buf) // if there's no data from the file and nothing left to parse, we're done
			return;
					
		// set dataEnd just past a splitting point -- look for a newline
		if (dataLength == maxReadSize && dataLength > 1) {
			char *pc;

			for(pc = bufReadStart + dataLength - 1; pc != buf; pc--) {
				if (pc[0] == '\n') {
					dataEnd = pc + 1; // include the newline
					break;
				}
			}

			if (pc == buf)
				dataEnd = bufReadStart + dataLength; // cool, we must be on the last line
		} else {
			dataEnd = bufReadStart + dataLength;
		}

		p = buf;
		pe = dataEnd;
		eof = pe;

		

	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _ma_actions + _ma_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 6:

	{ts = p;}
	break;

		}
	}

	_keys = _ma_trans_keys + _ma_key_offsets[cs];
	_trans = _ma_index_offsets[cs];

	_klen = _ma_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _ma_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
_eof_trans:
	cs = _ma_trans_targs[_trans];

	if ( _ma_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _ma_actions + _ma_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:

	{
		argument_start_mark = p;
	}
	break;
	case 1:

	{
		size_t len = (p - argument_start_mark) / sizeof(char);

		// copy the string into the buffer
		strncpy(argument_buffer, argument_start_mark, len);
		argument_buffer[len] = '\0';

		// now unquote it (substituting for string escapes) and find the CRC-32 of the unquoted string
		unquote(unquoted_argument_buffer, argument_buffer);
		last_argument_crc = crc32(0L, (unsigned char *)unquoted_argument_buffer, (unsigned int)strlen(unquoted_argument_buffer));
		last_argument_hex_present = FALSE;

		if (writeOutValues)
			line_buffer_len += sprintf(line_buffer + line_buffer_len, "CRC(\"%s\", 0x%08X)", argument_buffer, last_argument_crc);
	}
	break;
	case 2:

	{
		argument_start_mark = p;
	}
	break;
	case 3:

	{
		if (!writeOutValues) {
			size_t len = (p - argument_start_mark) / sizeof(char);

			// copy the string into the buffer
			strncpy(argument_buffer, argument_start_mark, len);
			argument_buffer[len] = '\0';

			last_argument_hex = strtoul(argument_buffer, NULL, 16);
			last_argument_hex_present = TRUE;
		}
	}
	break;
	case 7:

	{te = p+1;}
	break;
	case 8:

	{te = p+1;{
		if (!writeOutValues) {
			if (!last_argument_hex_present || last_argument_hex != last_argument_crc)
				numMissingValues++;
		}
	}}
	break;
	case 9:

	{te = p+1;{
		if (writeOutValues) {
			line_buffer[line_buffer_len] = '\0';
			fprintf(outputFile, "%s\n", line_buffer);
			line_buffer_len = 0;
		}
	}}
	break;
	case 10:

	{te = p+1;{
		if (writeOutValues)
			line_buffer[line_buffer_len++] = *p;
	}}
	break;
	case 11:

	{te = p;p--;{
		if (writeOutValues)
			line_buffer[line_buffer_len++] = *p;
	}}
	break;
	case 12:

	{{p = ((te))-1;}{
		if (writeOutValues)
			line_buffer[line_buffer_len++] = *p;
	}}
	break;

		}
	}

_again:
	_acts = _ma_actions + _ma_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 5:

	{ts = 0;}
	break;

		}
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _ma_actions + _ma_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	if ( _ma_eof_trans[cs] > 0 ) {
		_trans = _ma_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 4:

	{
		if (writeOutValues) {
			line_buffer[line_buffer_len] = '\0';
			fprintf(outputFile, "%s\n", line_buffer);
			line_buffer_len = 0;
		}
	}
	break;

		}
	}
	}

	}


		// move anything from dataEnd to the end of the buffer into the beginning
		if (dataEnd != bufReadStart + dataLength) {
			size_t remainder = READ_BUF_SIZE - (size_t)(dataEnd - buf);
			memmove(buf, dataEnd, remainder);
			bufReadStart = buf + remainder;
		} else {
			bufReadStart = buf;
		}
	}
}

// unquote() converts a string containing escaped characters to the literal string it specifies.
#define ISOCTDIGIT(x) ((x) >= '0' && (x) <= '7')
#define ISHEXDIGIT(x) (((x) >= '0' && (x) <= '9') || (tolower(x) >= 'a' && tolower(x) <= 'f'))
#define TOHEXDIGIT(x) ((x) > '9' ? (10 + (tolower(x) - 'a')) : ((x) - '0'))

void unquote(char *unquotedString, const char *quotedString)
{
	const char *s = quotedString;
	char *d = unquotedString;

	while (*s != '\0') {
		if (*s == '\\' && s[1] != '\0') {
			// check for octal or hex format, otherwise do single-character escape
			if(ISOCTDIGIT(s[1]) && ISOCTDIGIT(s[2]) && ISOCTDIGIT(s[3])) {
				*d++ = ((s[1]) - '0') * 64 + ((s[2]) - '0') * 8 + ((s[3]) - '0');
				s += 4;
			} else if (s[1] == 'x' && ISHEXDIGIT(s[2]) && ISHEXDIGIT(s[3])) {
				*d++ = TOHEXDIGIT(s[2]) * 16 + TOHEXDIGIT(s[3]);
				s += 4;
			} else {
				if (s[1] == 'n')
					*d++ = '\n';
				else if (s[1] == 'r')
					*d++ = '\r';
				else if (s[1] == 'f')
					*d++ = '\f';
				else if (s[1] == 't')
					*d++ = '\t';
				else if (s[1] == 'a')
					*d++ = '\a';
				else if (s[1] == 'v')
					*d++ = '\v';
				else if (s[1] == '\n')
					*d++; // consume escaped newlines
				else
					*d++ = s[1];
				s += 2;
			}
		} else {
			*d++ = *s++;
		}
	}

	*d++ = '\0';
}
