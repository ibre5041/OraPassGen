#include "crypto.h"
#include "common.h"

#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <iostream>

#include <string.h>

using namespace std;

string genpasswd(string const& dbid, string const& passphrase, string const& n_str)
{
	static BN_CTX *ctx = BN_CTX_new();
	string retval;
	char *a_o_char(0), *p_o_char(0), *n_o_char(0), *r_o_char(0);
	int rc;

	// use password hash as number "a"
	unsigned char a_md5[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)passphrase.c_str(), passphrase.size(), a_md5);
	BIGNUM *a_bn = BN_bin2bn(a_md5, MD5_DIGEST_LENGTH, NULL);
	if (verbose_flag)
		printf("m %s\n", a_o_char = BN_bn2hex(a_bn));

	// use dbid as exponent "p" number
	BIGNUM *p_bn = BN_new();
	rc = BN_dec2bn(&p_bn, dbid.c_str());
	if (verbose_flag)
		printf("p %s\n", p_o_char = BN_bn2dec(p_bn));

	// convert n_str into n number
	BIGNUM *n_bn = BN_new();
	rc = BN_dec2bn(&n_bn, n_str.c_str());
	if (verbose_flag)
		printf("n %s\n", n_o_char = BN_bn2dec(n_bn));

	// compute r = a ^ p mod n
	BIGNUM *r_bn = BN_new();
	rc = BN_mod_exp(r_bn, a_bn, p_bn, n_bn, ctx);
	if (verbose_flag)
		printf("r %s\n", r_o_char = BN_bn2dec(r_bn));

	// compute sha(r)
	int r_len = BN_num_bytes(r_bn);
	unsigned char* r_m_bin = (unsigned char*)calloc(1, r_len);
	rc = BN_bn2bin(r_bn, r_m_bin);
	unsigned char r_sha[SHA224_DIGEST_LENGTH];
	SHA224(r_m_bin, r_len, r_sha);

	size_t r2_m_char_len = 4 * (SHA224_DIGEST_LENGTH / 3) + (SHA224_DIGEST_LENGTH % 3 != 0 ? 4 : 0);
	char *r2_m_char = (char*)calloc(1, r2_m_char_len);
	size_t r2_len;
	// base64 encode sha224(r)
	r2_m_char = base64_encode(r_sha, SHA224_DIGEST_LENGTH, &r2_len);
	r2_m_char[GENPW] = 0; // truncate to max password len

	{
		for (size_t i=0; i < GENPW; i++)
			if ( r2_m_char[i] == '/' || r2_m_char[i] == '+')
				r2_m_char[i] = '_'; // comply with Oracle password allowed chars

		// 1st character of password must be uppercase alpha not a digit
		if (r2_m_char[0] == '_')
			r2_m_char[0] = 'A';
		if (isdigit(r2_m_char[0]))
			r2_m_char[0] = 'A' + (r2_m_char[0] - '0');
		if (isalpha(r2_m_char[0]) && islower(r2_m_char[0]))
			r2_m_char[0] = 'A' + (r2_m_char[0] - 'a');

		// 2st character of password must be lowercase alpha
		if (r2_m_char[1] == '_')
			r2_m_char[1] = 'a';
		if (isdigit(r2_m_char[1]))
			r2_m_char[1] = 'a' + (r2_m_char[1] - '0');
		if (isalpha(r2_m_char[1]) && isupper(r2_m_char[1]))
			r2_m_char[1] = 'a' + (r2_m_char[1] - 'A');

		// 3rd character of password must be digit
		if (!isdigit(r2_m_char[2]))
			r2_m_char[2] = '0';
	}

	if (verbose_flag)
		printf("r2 %s %d\n", r2_m_char, strnlen(r2_m_char, r2_m_char_len));

	retval = r2_m_char;

	free(r2_m_char);
	free(r_m_bin);
	OPENSSL_free(r_o_char);	
	OPENSSL_free(n_o_char);
	OPENSSL_free(p_o_char);
	OPENSSL_free(a_o_char);

	BN_free(r_bn);
	BN_free(a_bn);
	BN_free(n_bn);
	return retval;
}
