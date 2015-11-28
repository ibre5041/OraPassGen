#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <getopt.h>
#include <dlfcn.h>
#ifdef ORACLE_FOUND
#include "trotl.h"
using namespace trotl;
#endif
#else
#include "getopt_long.h"
#endif

#include <string>
#include <ostream>
#include <iostream>

#include "common.h"
#include "crypto.h"
#include "dbutils.h"

using namespace std;

static void usage()
{
	printf(
		"                                      \n"
		"Usage:                                \n"
		"  --dbid        <numeric database id> \n"
		"  --passphrase  <passphrase>          \n"
		"  --verbose                           \n"
		"                                      \n"
		);
}

extern "C" {
	extern char _binary_resources_n_txt_start;
	extern char _binary_resources_n_txt_size;
	extern char _binary_resources_n_txt_end;
}

int main(int argc, char *argv[])
{
	std::string passphrase, dbid;
	unsigned passphrase_length;

	while (1)
	{
		static struct option long_options[] = {
			/* These options set a flag. */
			{"verbose", no_argument,       &verbose_flag, 1},
			/* These options don’t set a flag.
			   We distinguish them by their indices. */
			{"dbid",       required_argument, 0, 'I'},
			{"passphrase", required_argument, 0, 'P'},
			{ "help",      no_argument,       0, 'h' },
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;
		int c = getopt_long (argc, argv, "hI:P:", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			break;
		case 'I':
			dbid = optarg;
			break;
		case 'P':
			passphrase = optarg;
			{ // hide passphrase from commandline
				size_t idx = 0;
				while(optarg[idx])
				{
					optarg[idx++] = '*';
				}
			}
			break;
		case 'h':
			usage();
			return 0;
		case '?':
			/* getopt_long already printed an error message. */
			break;
		default:
			abort ();
		}
	}
	if (verbose_flag)
		puts ("verbose flag is set");

	if (passphrase.empty())
	{
		char pw1[MAXPW] = {0}, pw2[MAXPW] = {0};
		char *p1 = pw1, *p2 = pw2;
		ssize_t nchr = 0;
		std::string passphrase2;
		printf ( "\n Enter passphrase:  ");
		nchr = getpasswd (&p1, MAXPW, '*', stdin);
		printf ( "\n Retype passphrase: ");
		nchr = getpasswd (&p2, MAXPW, '*', stdin);
		printf("\n----------------------------\n");
		if (verbose_flag) {
			printf("\n you entered   : %s  (%zu chars)\n", p1, nchr);
			printf("\n you entered   : %s  (%zu chars)\n", p2, nchr);
		}
		passphrase = pw1;
		passphrase2 = pw2;
		if (passphrase != passphrase2)
		{
			printf("passphrases do not match\n");
			return 2;
		}		
		passphrase_length = nchr;
	}

#if defined(__unix__) && defined(ORACLE_FOUND)
	if (dbid.empty())
	try
	{ // connect / as sysdba
		OciEnvAlloc _envalloc;
		OciEnv _env(_envalloc);
		OciLogin _login(_env);
		OciConnection _con(_env, _login);
		SqlStatement q0(_con, "select dbid, name from v$database");
		std::string name, sid;
		while(!q0.eof())
		{
			q0 >> dbid >> name;
			std::cout << " dbid: " << dbid << std::endl;
			std::cout << " name: " << name << std::endl;
		}
		SqlStatement q1(_con, "select instance_name from v$instance");
		while(!q1.eof())
		{
			q1 >> sid;
			std::cout << " sid:  " << sid << std::endl;
		}
	}
	catch (OciException const& e)
	{
		std::cerr << e.what();
		return 1;
	}			
#endif

	string n_str;
#ifdef __linux__	
	void* elf_handle = dlopen(0,RTLD_NOW|RTLD_GLOBAL); // dlopen self
	// - generate factor file:           genn --decimal -f n.txt
	// - compile n.txt into .elf format: objcopy --input binary --output elf64-x86-64 --binary-architecture i386 resources/n.txt n.o
	// - append n.o src/CMakeLists.txt common_sources variable
	size_t n_len = (size_t) dlsym(elf_handle, "_binary_resources_n_txt_size");
	char *n_data = (char *) dlsym(elf_handle, "_binary_resources_n_txt_start");
	if (n_len)
	{ // 1st check whether n was compiled into this binary		
		n_str = std::string(n_data, n_len);
	}
#endif
	if (n_str.empty())
	{ // 2n read n from file		
		char *buffer;
		long int buffer_len;
		{
			FILE* file = fopen(N_FILE_DEC, "r");
			if (!file)
			{
				fprintf(stderr, "File not found: " N_FILE_DEC "\n");
				return 3;
			}
			fseek(file, 0L, SEEK_END);
			buffer_len = ftell(file);
			buffer = (char*)malloc(buffer_len);
			fseek(file, 0L, SEEK_SET);
			size_t read = fread(buffer, buffer_len, 1, file);
			fclose(file);
		}
		BIGNUM *n = BN_new();
		int rc = BN_dec2bn(&n, buffer);
		char *n_char = BN_bn2dec(n);
		if (verbose_flag)
			printf("n %s\n", n_char);
		n_str = n_char;
		free(buffer);
		OPENSSL_free(n_char);
		BN_free(n);
	}

	std::string gen_password = genpasswd(dbid, passphrase, n_str);
	
	std::cout << std::endl
		  << " " << "alter user sys identified by \"" << gen_password << "\";" << std::endl
		  << std::endl;		
}
