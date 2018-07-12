#include "common.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

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

#if defined(HAVE_GITREVISION_H)
# include "gitrevision.h"
#endif

std::string version_string()
{
	using namespace std;
	stringstream  version;
#if defined(HAVE_GITREVISION_H)
	version << left << setw(18) << "GITVERSION:" <<GITVERSION             << endl;
	version << left << setw(18) << "GITVERSION_MAJOR:" <<GITVERSION_MAJOR << endl;
	version << left << setw(18) << "GITVERSION_MINOR:" <<GITVERSION_MINOR << endl;
	version << left << setw(18) << "GIT_BUILD_TYPE:" <<GIT_BUILD_TYPE     << endl;
	version << left << setw(18) << "GITVERSION_COUNT:" <<GITVERSION_COUNT << endl;
	version << left << setw(18) << "GITVERSION_SHA1:" <<GITVERSION_SHA1   << endl;
	version << left << setw(18) << "GITVERSION_SHORT:" <<GITVERSION_SHORT << endl;
	version << left << setw(18) << "GIT_BRANCH:" <<GIT_BRANCH             << endl;
	version << left << setw(18) << "BUILD_TAG:" <<BUILD_TAG               << endl;
	version << left << setw(18) << "BUILD_DATE:" <<BUILD_DATE             << endl;
#else
	version << "0.9" << endl;
#endif
	return version.str();
}

void prompt_passphrase(std::string &passphrase)
{
    char pw1[MAXPW] = {0}, pw2[MAXPW] = {0};
    char *p1 = pw1, *p2 = pw2;
    ssize_t nchr = 0;
    std::string passphrase1, passphrase2;
    printf ( "\n Enter passphrase:  ");
    nchr = getpasswd (&p1, MAXPW, '*', stdin);
    printf ( "\n Retype passphrase: ");
    nchr = getpasswd (&p2, MAXPW, '*', stdin);
    printf("\n----------------------------\n");
    if (verbose_flag) {
        printf("\n you entered   : %s  (%zu chars)\n", p1, nchr);
        printf("\n you entered   : %s  (%zu chars)\n", p2, nchr);
    }
    passphrase1 = pw1;
	passphrase2 = pw2;
    if (passphrase1 != passphrase2)
    {
        printf("passphrases do not match\n");
        exit(2);
    }
	passphrase = passphrase1;
}

std::string slurp(const char *filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in.is_open())
    {
        std::stringstream sstr;
        sstr << in.rdbuf();
        return sstr.str();
    } else {
        return "";
    }
}

std::vector<unsigned char> hex2bytes(const std::string& hex) {
	std::vector<unsigned char> bytes;

	std::cout << "hex2bytes: ";
	for (unsigned int i = 0; i < hex.length(); i += 2) {
		std::string byteString = hex.substr(i, 2);
		char byte = (unsigned char) strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
		std::cout << std::hex << (unsigned int)byte << ":";
	}
	std::cout << std::endl;
	std::cout << std::dec;
	return bytes;
}
