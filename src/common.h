#pragma once

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <stdio.h>

//#define DBPASS_BITS 32
#define DBPASS_BITS 4096
#define DBPASS_BYTES (DBPASS_BITS / 8)

#define N_FILE_BIN "n.bin"
#define N_FILE_HEX "n.hex"

void begin_read_password();
void end_read_password();

#define MAXPW 32
ssize_t getpasswd (char **pw, size_t sz, int mask, FILE *fp);
