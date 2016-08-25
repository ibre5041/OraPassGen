#include "common.h"
#include <string>
#include <iostream>

int verbose_flag = false;

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
//#include <unistd.h>

std::string exec(std::string cmd) {
	FILE * pipe = _popen(cmd.c_str(), "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	std::string result = "";
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	_pclose(pipe);
	return result;
}

ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp)
{

	DWORD mode, count;
	size_t idx = 0;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleMode(hStdin, &mode))
	{
		// Set the console mode to no-echo, not-line-buffered input
		SetConsoleMode(hStdin, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
		char c;
		while (ReadConsoleA(hStdin, &c, 1, &count, NULL) && (c != '\r') && (c != '\n') && idx < sz)
		{
			if (c == '\b') // Backspace
			{
				if (idx)
				{
					WriteConsoleA(hStdout, "\b \b", 3, &count, NULL);
					(*pw)[--idx] = 0;
				}
			}
			else
			{
				WriteConsoleA(hStdout, "*", 1, &count, NULL);
				(*pw)[idx++] = c;
			}
		}
		// Restore the console mode
		SetConsoleMode(hStdin, mode);
		if (verbose_flag)
			std::cout << std::endl << "Console mode restored" << std::endl;
	} else {
		// Cygwin path
		char c = 0;
		fflush(stdout);
		std::string current = exec("stty -g");
		exec("stty raw -echo");

		while (_read(0, &c, 1) && (c != '\r') && (c != '\n') && idx < sz)
		{
			if (c == '\b') // Backspace
			{
				if (idx)
				{
					//_write(2, "\b \b", 3);
					(*pw)[--idx] = 0;
				}
			}
			else
			{
				_write(2, "*", 1);
				(*pw)[idx++] = c;
			}
		}
		exec("stty " + current);
	}

	(*pw)[idx] = 0; /* null-terminate   */
	return idx;
}

#else

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <termios.h>
#include <errno.h>   /* for errno */
#include <unistd.h>  /* for EINTR */

/* read a string from fp into pw masking keypress with mask char.
   getpasswd will read upto sz - 1 chars into pw, null-terminating
   the resulting string. On success, the number of characters in
   pw are returned, -1 otherwise.
 */
ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp)
{
	if (!pw || !sz || !fp) return -1;       /* validate input   */
#ifdef MAXPW
	if (sz > MAXPW) sz = MAXPW;
#endif

	if (*pw == NULL) {              /* reallocate if no address */
		void *tmp = realloc (*pw, sz * sizeof **pw);
		if (!tmp)
			return -1;
		memset (tmp, 0, sz);    /* initialize memory to 0   */
		*pw = (char*)tmp;
	}

	size_t idx = 0;         /* index, number of chars in read   */
	int c = 0;

	struct termios old_kbd_mode;    /* orig keyboard settings   */
	struct termios new_kbd_mode;

	if (tcgetattr (0, &old_kbd_mode)) { /* save orig settings   */
		fprintf (stderr, "%s() error: tcgetattr failed.\n", __func__);
		return -1;
	}   /* copy old to new */
	memcpy (&new_kbd_mode, &old_kbd_mode, sizeof(struct termios));

	new_kbd_mode.c_lflag &= ~(ICANON | ECHO);  /* new kbd flags */
	new_kbd_mode.c_cc[VTIME] = 0;
	new_kbd_mode.c_cc[VMIN] = 1;
	if (tcsetattr (0, TCSANOW, &new_kbd_mode)) {
		fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
		return -1;
	}

	/* read chars from fp, mask if valid char specified */
	while (((c = fgetc (fp)) != '\n' && c != EOF && idx < sz - 1) ||
			(idx == sz - 1 && c == 127))
	{
		if (c != 127) {
			if (31 < mask && mask < 127)    /* valid ascii char */
				fputc (mask, stdout);
			(*pw)[idx++] = c;
		}
		else if (idx > 0) {         /* handle backspace (del)   */
			if (31 < mask && mask < 127) {
				fputc (0x8, stdout);
				fputc (' ', stdout);
				fputc (0x8, stdout);
			}
			(*pw)[--idx] = 0;
		}
	}
	(*pw)[idx] = 0; /* null-terminate   */

	/* reset original keyboard  */
	if (tcsetattr (0, TCSANOW, &old_kbd_mode)) {
		fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
		return -1;
	}

	if (idx == sz - 1 && c != '\n') /* warn if pw truncated */
		fprintf (stderr, " (%s() warning: truncated at %zu chars.)\n",
				__func__, sz - 1);

	return idx; /* number of chars in passwd    */
}

#endif

#include <stdint.h>
#include <stdlib.h>


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
				'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
				'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
				'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
				'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
				'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
				'w', 'x', 'y', 'z', '0', '1', '2', '3',
				'4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};


char *base64_encode(const unsigned char *data,
		    size_t input_length,
		    size_t *output_length)
{

	*output_length = 4 * ((input_length + 2) / 3);

	char *encoded_data = (char*)calloc(1, *output_length);
	if (encoded_data == NULL) return NULL;

	for (int i = 0, j = 0; i < input_length;) {

		uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[*output_length - 1 - i] = '=';

	return encoded_data;
}
