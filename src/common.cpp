#include "common.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

void begin_read_password()
{
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#else
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
}

void end_read_password()
{
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
#else
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
}

#ifdef _WIN32
#include <windows.h>
#include<conio.h>

ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp)
{
	while( ( pass[i]=getch() ) != '\n' && pass[i] != '\r' && i<19 )
	{putchar('*'); i++;}
	pass[i]='\0';
	cout<<endl;
}

#else

#include <stdlib.h>
#include <string.h>

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
