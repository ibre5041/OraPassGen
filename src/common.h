#pragma once

#include <string>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

//#define DBPASS_BITS 32
#define DBPASS_BITS 4096
#define DBPASS_BYTES (DBPASS_BITS / 8)

#define N_FILE_DEC "n.txt"
#define N_FILE_BIN "n.bin"
#define N_FILE_HEX "n.hex"

#define MAXPW 32
#define GENPW MD5_DIGEST_LENGTH
ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp);

extern int verbose_flag;
