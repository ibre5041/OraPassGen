#pragma once

#include <string>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

//#define DBPASS_BITS 32
#define DBPASS_BITS 4096
#define DBPASS_BYTES (DBPASS_BITS / 8)

#define N_FILE_DEC "n.txt"
#define N_FILE_BIN "n.bin"
#define N_FILE_HEX "n.hex"

#define MAXPW 32
#define GENPW 30
ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp);
char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length);

extern int verbose_flag;
