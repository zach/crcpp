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

%%{
	machine ma;

	action on_argument_start {
		argument_start_mark = p;
	}

	action on_argument {
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

	action check_value {
		if (!writeOutValues) {
			if (!last_argument_hex_present || last_argument_hex != last_argument_crc)
				numMissingValues++;
		}
	}

	action on_hex_number_start {
		argument_start_mark = p;
	}

	action on_hex_number {
		if (!writeOutValues) {
			size_t len = (p - argument_start_mark) / sizeof(char);

			// copy the string into the buffer
			strncpy(argument_buffer, argument_start_mark, len);
			argument_buffer[len] = '\0';

			last_argument_hex = strtoul(argument_buffer, NULL, 16);
			last_argument_hex_present = TRUE;
		}
	}

	action copy_char {
		if (writeOutValues)
			line_buffer[line_buffer_len++] = *p;
	}
	
	action output_line {
		if (writeOutValues) {
			line_buffer[line_buffer_len] = '\0';
			fprintf(outputFile, "%s\n", line_buffer);
			line_buffer_len = 0;
		}
	}

	hex_number	= '0x' . xdigit+;
	string_char = [^"\\] | /\\./; # match any non-quote or any escaped character

	crc_macro_identifier = 'CRC';

	crc_macro_unquoted_string = (string_char* >on_argument_start %on_argument);
	crc_macro_string = '"' . crc_macro_unquoted_string . '"';

	crc_macro_hex_arg = (',' . space* . hex_number >on_hex_number_start %on_hex_number . space*);
	
	crc_macro = crc_macro_identifier . '(' . space* . crc_macro_string . space* . crc_macro_hex_arg? . ')';

	main := |*
		crc_macro >/ output_line => check_value;
		'\n' => output_line;
		any >/ output_line => copy_char;
	*|;
}%%


%% write data;


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
			%% write init;

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

		%% write exec;

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
