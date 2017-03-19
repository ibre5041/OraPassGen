#include "crypto.h"
#include "common.h"

#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <boost/multiprecision/cpp_int.hpp>

#include "md5.h"
#include "sha224.h"

#include <vector>
#include <iostream>
#include <algorithm>

#include <string.h>

using namespace std;

string genpasswd(string const& dbid, string const& _username, string const& passphrase, string const& n_str)
{
	string username(_username);
	transform(username.begin(), username.end(), username.begin(), ::toupper);

	static BN_CTX *ctx = BN_CTX_new();
	string retval;
	char *a_o_char(0), *p_o_char(0), *n_o_char(0), *r_o_char(0), *u_o_char(0);
	int rc;

	// use password hash as number "a"
	unsigned char a_md5[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)passphrase.c_str(), passphrase.size(), a_md5);
	BIGNUM *a_bn = BN_bin2bn(a_md5, MD5_DIGEST_LENGTH, NULL);
	if (verbose_flag)
	{
		printf("a %s\n", a_o_char = BN_bn2hex(a_bn));
		printf("a %s\n", a_o_char = BN_bn2dec(a_bn));
	}


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
	{
		printf("r %s\n", r_o_char = BN_bn2dec(r_bn));
		OPENSSL_free(r_o_char);	
	}

	// use username hash as number "u"
	// different password for each user
	unsigned char u_md5[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)username.c_str(), username.size(), u_md5);
	BIGNUM *u_bn = BN_bin2bn(u_md5, MD5_DIGEST_LENGTH, NULL);
	if (verbose_flag)
	{
		printf("u %s\n", u_o_char = BN_bn2hex(u_bn));
		printf("u %s\n", u_o_char = BN_bn2dec(u_bn));
	}

	BIGNUM *r1_bn = BN_new();
	{
		BN_copy(r1_bn, r_bn);
		rc = BN_mod_exp(r_bn, r1_bn, u_bn, n_bn, ctx);
	}

 	// just burn some CPU time
	// compute r = a ^ r mod n
	for (int i=0; i<100; i++)
	{
		BN_copy(r1_bn, r_bn);
		rc = BN_mod_exp(r_bn, a_bn, r1_bn, n_bn, ctx);
	}
	if (verbose_flag)
	{
	    printf("r %s\n", r_o_char = BN_bn2dec(r_bn));
	    OPENSSL_free(r_o_char);
	    printf("r1 %s\n", r_o_char = BN_bn2dec(r1_bn));
	    OPENSSL_free(r_o_char);
	}

	// compute sha(r)
	int r_len = BN_num_bytes(r_bn);
	unsigned char* r_m_bin = (unsigned char*)calloc(1, r_len);
	rc = BN_bn2bin(r_bn, r_m_bin);
	unsigned char r_sha[SHA224_DIGEST_LENGTH];
	SHA224(r_m_bin, r_len, r_sha);

	size_t r2_m_char_len = 4 * (SHA224_DIGEST_LENGTH / 3) + (SHA224_DIGEST_LENGTH % 3 != 0 ? 4 : 0);
	char *r2_m_char;
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
		printf("r2 %s %zd\n", r2_m_char, strnlen(r2_m_char, r2_m_char_len));

	retval = r2_m_char;

	free(r2_m_char);
	free(r_m_bin);
	OPENSSL_free(n_o_char);
	OPENSSL_free(p_o_char);
	OPENSSL_free(a_o_char);
	OPENSSL_free(u_o_char);
	BN_free(r_bn);
	BN_free(a_bn);
	BN_free(n_bn);
	BN_free(p_bn);
	BN_free(u_bn);
	return retval;
}

#ifdef USE_BOOT_MULTI
// boost::multiprecision implementation
// easier to read. but implementation is MUCH SLOWER than openssl big_num
//
string genpasswd2(string const& dbid, string const& _username, string const& passphrase, string const& n_str)
{
    string retval;

    string username(_username);
    transform(username.begin(), username.end(), username.begin(), ::toupper);

    Crypto::MD5 A_MD5(passphrase);
    boost::multiprecision::cpp_int A;
    {
        std::vector<unsigned char> v;
        for(unsigned i=0; i < 16; ++i)
        {
            v.push_back(A_MD5.bindigest()[i]);
        }
        boost::multiprecision::import_bits(A, v.begin(), v.end());
    }
    std::cout << "A " << A_MD5.hexdigest() << std::endl;
    std::cout << "A " << A << std::endl;


    // use dbid as exponent "p" number
    boost::multiprecision::cpp_int P(dbid);
    std::cout << "P " << P << std::endl;

    // convert n_str into n number
    boost::multiprecision::cpp_int N(n_str);
    std::cout << "N " << N << std::endl;

    // compute r = a ^ p mod n
    boost::multiprecision::cpp_int R = powm(A, P, N);
    std::cout << "R " << R << std::endl;


    // use username hash as number "u"
    // different password for each user
    Crypto::MD5 U_MD5(username);
    boost::multiprecision::cpp_int U;
    {
        std::vector<unsigned char> v;
        for (unsigned i = 0; i < 16; ++i)
        {
            v.push_back(U_MD5.bindigest()[i]);
        }
        boost::multiprecision::import_bits(U, v.begin(), v.end());
    }
    std::cout << "U " << U_MD5.hexdigest() << std::endl;
    std::cout << "U " << U << std::endl;

    boost::multiprecision::cpp_int R1 = R;
    R = powm(R1, U, N);

    // just burn some CPU time
    // compute r = a ^ r mod n
    for (int i=0; i<100; i++)
    {
        R1 = R;
        R = powm(A, R1, N);
    }
    std::cout << "R " << R << std::endl;
    std::cout << "R1 " << R1 << std::endl;

    // compute sha(r)
    std::string v;
    boost::multiprecision::export_bits(R, std::back_inserter(v), 8);
    Crypto::SHA224 R2_SHA224(v);
    std::string R2_bin((const char*)R2_SHA224.bindigest());

    size_t r2_len;
    char *r2_m_char = base64_encode((const unsigned char *)R2_bin.c_str(), SHA224_DIGEST_LENGTH, &r2_len);
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


    std::cout << "r2 " << r2_m_char << std::endl;

    retval = r2_m_char;
    return retval;
}
#endif
