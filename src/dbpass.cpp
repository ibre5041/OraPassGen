#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "trotl.h"

#include <string>
#include <ostream>

#include "common.h"
#include "dbutils.h"

using namespace trotl;
using namespace std;

int main(int argc, char *argv[])
{
	/* Flag set by "--verbose". */
	static int verbose_flag = 0;
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
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;
		int c = getopt_long (argc, argv, "I:P:", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			printf ("option %s", long_options[option_index].name);
			if (optarg)
				printf (" with arg %s", optarg);
			printf ("\n");
			break;
		case 'I':
			printf ("option -I with value `%s'\n", optarg);
			dbid = optarg;
			break;
		case 'P':
			printf ("option -P with value `%s'\n", optarg);
			password = optarg;
			break;
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
		printf ("\n you entered   : %s  (%zu chars)\n", p, nchr);
		password = pw;
		password_length = nchr;
	}

	if (dbid.empty())
	try
	{ // connect / as sysdba
		OciEnvAlloc _envalloc;
		OciEnv _env(_envalloc);
		OciLogin _login(_env);
		OciConnection _con(_env, _login);
		SqlStatement q0(_con, "select dbid from v$database");
		std::string su;
		while(!q0.eof())
		{
			q0 >> su;
			std::cout << "dbid: " << su << std::endl;
		}
		dbid = su;
	}
	catch (OciException const& e)
	{
		std::cerr << e.what();
		return -1;
	}			

	BN_CTX *ctx = BN_CTX_new();
	int rc;

	// use password hash has number a
	unsigned char *md5 = MD5((unsigned char*)password.c_str(), password_length, NULL);
	BIGNUM *a = BN_bin2bn(md5, 16, NULL);
	char *a_char = BN_bn2hex(a);
	printf("m %s\n", a_char);

	// use dbid as exponent p
	BIGNUM *p = BN_new();
	rc = BN_dec2bn(&p, dbid.c_str());
	char *p_char = BN_bn2dec(p); 
	printf("p %s\n", p_char);

	// read n from file
	unsigned char *buffer;
	long int buffer_len;
	{
		FILE* file;
		file = fopen("genn.bin", "r");
		fseek(file, 0L, SEEK_END);
		buffer_len = ftell(file);
		buffer = (unsigned char*)malloc(buffer_len);
		fseek(file, 0L, SEEK_SET);
		size_t read = fread(buffer, buffer_len, 1, file);
		fclose(file);
	}
	BIGNUM *n = BN_new();
	n = BN_bin2bn(buffer, buffer_len, n);
	char *n_char = BN_bn2dec(n);
	printf("n %s\n", n_char);

	BIGNUM *r = BN_new();
	rc = BN_mod_exp(r, a, p, n, ctx);
	char *r_char = BN_bn2dec(r);
	printf("r %s\n", r_char);
	
	OPENSSL_free(a_char);
	OPENSSL_free(p_char);
	OPENSSL_free(n_char);
	OPENSSL_free(r_char);
	BN_CTX_free(ctx);
	BN_free(r);
	BN_free(a);
	BN_free(n);
}

#if 0
	BIO *bio, *b64;
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_fp(stdout, BIO_NOCLOSE);
	BIO_push(b64, bio);
	BIO_write(b64, md5, 16);
	BIO_flush(b64);
	BIO_free_all(b64);
#endif
