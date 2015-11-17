#include "crypto.h"

#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include <iostream>

using namespace std;

string genpasswd(string const& dbid, string const& passphrase, string const& n_str)
{
	string retval;

	BN_CTX *ctx = BN_CTX_new();
	int rc;

	// use password hash as number "a"
	unsigned char a_md5[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)passphrase.c_str(), passphrase.size(), a_md5);
	BIGNUM *a = BN_bin2bn(a_md5, MD5_DIGEST_LENGTH, NULL);
	char *a_char = BN_bn2hex(a);
	printf("m %s\n", a_char);

	// use dbid as exponent "p" number
	BIGNUM *p = BN_new();
	rc = BN_dec2bn(&p, dbid.c_str());
	char *p_char = BN_bn2dec(p);
	printf("p %s\n", p_char);

	BIGNUM *n = BN_new();
	rc = BN_dec2bn(&n, n_str.c_str());
	char *n_char = BN_bn2dec(n);
	printf("n %s\n", n_char);

	BIGNUM *r = BN_new();
	rc = BN_mod_exp(r, a, p, n, ctx);
	char *r_char = BN_bn2dec(r);
	printf("r %s\n", r_char);

	int r_len = BN_num_bytes(r);
	unsigned char* r_bin = (unsigned char*)malloc(r_len);
	rc = BN_bn2bin(r, r_bin);
	unsigned char r_md5[MD5_DIGEST_LENGTH];
	MD5(r_bin, r_len, r_md5);

	char mdString[33];
	for (int i = 0; i < 16; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)r_md5[i]);
	mdString[32] = 0;
	BIGNUM *r2 = BN_bin2bn(r_md5, MD5_DIGEST_LENGTH, NULL);
	char *r2_char = BN_bn2hex(r2);

	printf("r2 a %s\n", mdString);
	printf("r2 b %s\n", r2_char);

	retval = mdString;

	free(r_bin);
	OPENSSL_free(a_char);
	OPENSSL_free(p_char);
	OPENSSL_free(n_char);
	OPENSSL_free(r_char);
	BN_CTX_free(ctx);
	BN_free(r2);
	BN_free(r);
	BN_free(a);
	BN_free(n);
	return retval;
}
