#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#ifdef _WIN32
#include "getopt_long.h"
#else
#include <getopt.h>
#endif
#include <stdio.h>
#include <string.h>

#include <string>

#include "common.h"

static void usage()
{
	printf(
		"                  \n"
		"Usage:            \n"
		"  --help          \n"
		"  --base64        \n"
		"  --binary        \n"
		"  --decimal       \n"
		"  --hexa          \n"
		"  --file filename \n"
		"  --verbose       \n"
		"                  \n"
		);
}

int main(int argc, char *argv[])
{
	int rc;
	
	RAND_poll();

	BIGNUM *prime_p = BN_new();
	BIGNUM *prime_q = BN_new();
	BIGNUM *n = BN_new();

	BN_CTX *ctx = BN_CTX_new();
	RSA *r = RSA_new();
	BIGNUM *bne = BN_new();
	
	char *prime_p_char, *prime_q_char, *n_char;
	
	/* Flag set by "--verbose". */
	bool base64_flg, binary_flg, decimal_flg, hex_flg;
	base64_flg = binary_flg = decimal_flg = hex_flg = false;
	std::string filename;
	int c;
	while (1)
	{
		static struct option long_options[] = {
			/* These options set a flag. */
			{"verbose", no_argument,       &verbose_flag, 1},
			/* These options don’t set a flag.
			   We distinguish them by their indices. */
			{"help",    no_argument,       0, 'h'},
			{"base64",  no_argument,       0, '6'},
			{"binary",  no_argument,       0, 'b'},
			{"decimal", no_argument,       0, 'd'},
			{"hexa",    no_argument,       0, 'x'},
			{"file",    required_argument, 0, 'f'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;
		c = getopt_long (argc, argv, "6bdhf:", long_options, &option_index);

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
		case 'h':
			usage();
			break;
		case '6':
			puts ("option --base64\n");
			base64_flg = true;
			break;			
		case 'b':
			puts ("option -b\n");
			binary_flg = true;
			break;
		case 'x':
			puts ("option -x\n");
			hex_flg = true;
			break;
		case 'd':
			puts ("option -d\n");
			decimal_flg = true;
			break;
		case 'f':
			printf ("option -f with value `%s'\n", optarg);
			filename = optarg;
			break;
		case '?':
			/* getopt_long already printed an error message. */
			break;
		default:
			abort ();
		}
	}

	if (!(base64_flg | binary_flg | decimal_flg | hex_flg))
	{
		usage();
		return 0;
	}

	if (verbose_flag)
		puts ("verbose flag is set");

#if 0
	// generate two random primes p,q (and multipy them then n = p*q)
	rc = BN_generate_prime_ex(prime_p, DBPASS_BITS, true, NULL, NULL, NULL);
	if(rc != 1){ goto free_all; }
	
	rc = BN_generate_prime_ex(prime_q, DBPASS_BITS, true, NULL, NULL, NULL);
	if(rc != 1){ goto free_all; }
	
	rc = BN_mul(n, prime_p, prime_q, ctx);
	if(rc != 1){ goto free_all; }

	prime_p_char = BN_bn2dec(prime_p);
	prime_q_char = BN_bn2dec(prime_q);
	n_char       = BN_bn2dec(n);
	
#else 	
	// generate rsa key (including p,q,n in it)
	unsigned long e = RSA_F4;
	rc = BN_set_word(bne,e);
	if(rc != 1){goto free_all;}

	rc = RSA_generate_key_ex(r, DBPASS_BITS, bne, NULL);
	if(rc != 1){goto free_all;}
	
	prime_p_char = BN_bn2dec(r->p);
	prime_q_char = BN_bn2dec(r->q);
	n_char       = BN_bn2dec(r->n);
	n = BN_dup(r->n);
#endif
	
	if (verbose_flag) {
		printf("Random prime %s\n", prime_p_char);
		printf("Random prime %s\n", prime_q_char);
	}
	printf("n %s\n", n_char);

	FILE* file;
	if (base64_flg)
	{

		char *n_hex;
		BIO *b64 = BIO_new(BIO_f_base64());
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
		n_hex = BN_bn2hex(n);
		//BIO_write()
	} else if (binary_flg)
	{
		int len = BN_num_bytes(n);
		unsigned char *n_bin = (unsigned char *)malloc(len);
		rc = BN_bn2bin(n, n_bin);
		if (filename.empty())
			file = fopen("genn.bin", "w");
		else
			file = fopen(filename.c_str(), "w");
		size_t written = fwrite(n_bin, len, 1, file);
		fflush(file);
		fclose(file);
	} else if (decimal_flg) {
		if (filename.empty())
			file = fopen("genn.dec.txt", "w");
		else
			file = fopen(filename.c_str(), "w");
		size_t written = fwrite(n_char, strlen(n_char), 1, file);
		fflush(file);
		fclose(file);
	} else if (hex_flg) {
		char *n_hex = BN_bn2hex(n);
		if (filename.empty())
			file = fopen("genn.hex.txt", "w");
		else
			file = fopen(filename.c_str(), "w");
		size_t written = fwrite(n_hex, strlen(n_hex), 1, file);
		OPENSSL_free(n_hex);
		fflush(file);
		fclose(file);
	}


	
free_all:
	RSA_free(r);
	OPENSSL_free(prime_p_char);
	OPENSSL_free(prime_q_char);
	OPENSSL_free(n_char);
	BN_CTX_free(ctx);
	BN_free(prime_p);
	BN_free(prime_q);
	BN_free(n);
	BN_free(bne);
}
