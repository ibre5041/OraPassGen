#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include <stdio.h>
#include <stdlib.h>


#ifndef _WIN32
#include <getopt.h>
#include "trotl.h"
using namespace trotl;
#else
#include "getopt_long.h"
#endif

#include <string>
#include <ostream>

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

int main(int argc, char *argv[])
{
	std::string password, dbid;
	unsigned password_length;

	while (1)
	{
		static struct option long_options[] = {
			/* These options set a flag. */
			{"verbose", no_argument,       &verbose_flag, 1},
			/* These options don’t set a flag.
			   We distinguish them by their indices. */
			{"dbid",     required_argument, 0, 'I'},
			{"password", required_argument, 0, 'P'},
			{ "help",    no_argument,       0, 'h' },
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
			password = optarg;
			{ // hide password from commandline
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

	if (password.empty())
	{
		char pw[MAXPW] = {0};
		char *p = pw;
		ssize_t nchr = 0;
		printf ( "\n Enter password: ");
		nchr = getpasswd (&p, MAXPW, '*', stdin);
		if (verbose_flag)
			printf("\n you entered   : %s  (%zu chars)\n", p, nchr);
		printf("\n----------------------------\n");
		password = pw;
		password_length = nchr;
	}

#ifndef _WIN32
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
		return -1;
	}			
#endif

	string n_str;
	{
		// read n from file
		char *buffer;
		long int buffer_len;
		{
			FILE* file = fopen(N_FILE_DEC, "r");
			if (!file)
			{
				fprintf(stderr, "File not found: " N_FILE_DEC "\n");
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
	std::string gen = genpasswd(dbid, password, n_str);

	printf("\n\n");
	printf("alter user sys identified by \"%s\";\n", gen.c_str());
}
