#include "crypto.h"
#include "common.h"

#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include <iostream>

using namespace std;

string genpasswd(string const& dbid, string const& passphrase, string const& n_str)
{
	static BN_CTX *ctx = BN_CTX_new();
	string retval;
	char *a_char(0), *p_char(0), *n_char(0), *r_char(0);
	int rc;

	// use password hash as number "a"
	unsigned char a_md5[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)passphrase.c_str(), passphrase.size(), a_md5);
	BIGNUM *a = BN_bin2bn(a_md5, MD5_DIGEST_LENGTH, NULL);
	if (verbose_flag)
		printf("m %s\n", a_char = BN_bn2hex(a));

	// use dbid as exponent "p" number
	BIGNUM *p = BN_new();
	rc = BN_dec2bn(&p, dbid.c_str());
	if (verbose_flag)
		printf("p %s\n", p_char = BN_bn2dec(p));

	// convert n_str into n number
	BIGNUM *n = BN_new();
	rc = BN_dec2bn(&n, n_str.c_str());
	if (verbose_flag)
		printf("n %s\n", n_char = BN_bn2dec(n));

	// compute r = a ^ p mod n
	BIGNUM *r = BN_new();
	rc = BN_mod_exp(r, a, p, n, ctx);
	if (verbose_flag)
		printf("r %s\n", r_char = BN_bn2dec(r));

	// compute md5(r)
	int r_len = BN_num_bytes(r);
	unsigned char* r_bin = (unsigned char*)malloc(r_len);
	rc = BN_bn2bin(r, r_bin);
	unsigned char r_md5[MD5_DIGEST_LENGTH];
	MD5(r_bin, r_len, r_md5);

	// get hexa representation of md5(r)
	BIGNUM *r2 = BN_bin2bn(r_md5, MD5_DIGEST_LENGTH, NULL);
	char *r2_char = BN_bn2hex(r2);

	{
		// 1st character of password must be a alpha not a digit
		if (isdigit(r2_char[0]))
		{
			r2_char[0] = 'A' + (r2_char[0] - '0');
		}

		// 2st character of password must be lowercase alpha
		if (isdigit(r2_char[1]))
		{
			r2_char[1] = 'a' + (r2_char[1] - '0');
		}
		if (isalpha(r2_char[1]) && isupper(r2_char[1]))
		{
			r2_char[1] = 'a' + (r2_char[1] - 'A');
		}
		// 3rd character of password must be digit
		if (!isdigit(r2_char[2]))
		{
			r2_char[2] = '0';
		}
	}

	if (verbose_flag)
		printf("r2 %s\n", r2_char);

	retval = r2_char;

	free(r_bin);
	OPENSSL_free(r2_char);
	OPENSSL_free(r_char);	
	OPENSSL_free(n_char);
	OPENSSL_free(p_char);
	OPENSSL_free(a_char);

	//BN_CTX_free(ctx);
	BN_free(r2);
	BN_free(r);
	BN_free(a);
	BN_free(n);
	return retval;
}
