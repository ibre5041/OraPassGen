#include "crypto.h"
#include "common.h"

#include "md5.h"
#include "sha224.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <string.h>
#include <limits.h>

using namespace std;

void read_key_env(std::string &passphrase)
{
	char *opass_key = getenv("OPASSGEN_KEY");
	if (!opass_key || strlen(opass_key) != 32) // 32 = MD5_DIGEST_LENGTH*2
		return;

	string f(opass_key);
	char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for (int i = 0; i < 16; i++)
	{
		char c = f.at(i);
		char j;
		if ('0' <= c && c <= '9')
			j = c - '0';
		else if ('a' <= c && c <= 'f')
			j = c - 'a' + 10;
		else if ('A' <= c && c <= 'F')
			j = c - 'A' + 10;
		else abort();
		j = j ^ 0xF;
		c = hex_chars[j];
		f[i] = c;
	}
	passphrase = f.c_str();
}

void write_keyfile(std::string const& passphrase)
{
	Crypto::MD5 MD5(passphrase);
	std::string a_md5_hex = MD5.hexdigest();
	std::ofstream out("key.txt", std::ios::trunc);

	char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for(int i=0; i<MD5_DIGEST_LENGTH; i++)
	{
		char c = a_md5_hex[i];
		char j;
		if ('0' <= c && c <= '9')
			j = c - '0';
		else if ('a' <= c && c <= 'f')
			j = c - 'a' + 10;
		else if ('A' <= c && c <= 'F')
			j = c - 'A' + 10;
		else abort();
		j = j ^ 0xF;
		a_md5_hex[i] = hex_chars[j];
	}
	out << a_md5_hex;

	out.close();
};

void read_keyfile(std::string &passphrase)
{
	std::string f = slurp("key.txt");
	if (f.empty())
		return;

	char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for (int i = 0; i<MD5_DIGEST_LENGTH; i++)
	{
		char c = f.at(i);
		char j;
		if ('0' <= c && c <= '9')
			j = c - '0';
		else if ('a' <= c && c <= 'f')
			j = c - 'a' + 10;
		else if ('A' <= c && c <= 'F')
			j = c - 'A' + 10;
		else abort();
		j = j ^ 0xF;
		c = hex_chars[j];
		f[i] = c;
	}

	passphrase = f.c_str();
};

#ifdef OPENSSL_FOUND
#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

string genpasswd_openssl(string const& dbid, string const& _username, string const& passphrase, string const& n_str)
{
	string username(_username);
	transform(username.begin(), username.end(), username.begin(), ::toupper);

	static BN_CTX *ctx = BN_CTX_new();
	string retval;
	char *a_o_char(0), *p_o_char(0), *n_o_char(0), *r_o_char(0), *u_o_char(0);
	int rc;

	// use password hash as number "a"
	unsigned char a_md5[MD5_DIGEST_LENGTH];
	BIGNUM *a_bn(0);
	if (passphrase.size() != MD5_DIGEST_LENGTH*2)
	{
	    // usual way passphrase passed from users input
	    MD5((unsigned char*)passphrase.c_str(), passphrase.size(), a_md5);
	    a_bn = BN_bin2bn(a_md5, MD5_DIGEST_LENGTH, NULL);
	    if (verbose_flag)
	    {
	        printf("a %s\n", a_o_char = BN_bn2hex(a_bn));
	        printf("a %s\n", a_o_char = BN_bn2dec(a_bn));
	    }
	} else {
	    // a "key way" passphrase read from keyfile
	    rc = BN_hex2bn(&a_bn, passphrase.c_str());
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

	if (verbose_flag)
	{
           printf("r_len %d\n", r_len);
	}

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

#endif

#if(BOOST_FOUND)
#include <boost/multiprecision/cpp_int.hpp>

// boost::multiprecision implementation
// easier to read. but implementation is MUCH SLOWER than openssl big_num
//
string genpasswd_boost(string const& dbid, string const& _username, string const& passphrase, string const& n_str)
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
    if (verbose_flag)
    {
	    std::cout << "A " << A_MD5.hexdigest() << std::endl;
	    std::cout << "A " << A << std::endl;
    }

    // use dbid as exponent "p" number
    boost::multiprecision::cpp_int P(dbid);
    if (verbose_flag)
	std::cout << "P " << P << std::endl;

    // convert n_str into n number
    boost::multiprecision::cpp_int N(n_str);
    if (verbose_flag)
	std::cout << "N " << N << std::endl;

    // compute r = a ^ p mod n
    boost::multiprecision::cpp_int R = powm(A, P, N);
    if (verbose_flag)
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
    if (verbose_flag)
    {
	    std::cout << "U " << U_MD5.hexdigest() << std::endl;
	    std::cout << "U " << U << std::endl;
    }
    boost::multiprecision::cpp_int R1 = R;
    R = powm(R1, U, N);

    // just burn some CPU time
    // compute r = a ^ r mod n
    for (int i=0; i<100; i++)
    {
        R1 = R;
        R = powm(A, R1, N);
    }
    if (verbose_flag)
    {
	    std::cout << "R " << R << std::endl;
	    std::cout << "R1 " << R1 << std::endl;
    }
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

    if (verbose_flag)
	    std::cout << "r2 " << r2_m_char << std::endl;

    retval = r2_m_char;
    return retval;
}
#endif

#ifdef MPIR_FOUND

#include <mpirxx.h>
#include <mpir.h>
string genpasswd_mpir(string const& dbid, string const& _username, string const& passphrase, string const& n_str)
{
	string retval;

	string username(_username);
	transform(username.begin(), username.end(), username.begin(), ::toupper);

	// use password hash as number "a"
	Crypto::MD5 A_MD5(passphrase);
	mpz_t a;
	mpz_init(a);
	if (passphrase.size() != MD5_DIGEST_LENGTH*2)
	{
		mpz_import (a, 16, 1, 1, 0, 0, A_MD5.bindigest());
		if (verbose_flag)
		{
			std::cout << "A " << A_MD5.hexdigest() << std::endl;	
		}		
	} else {
		mpz_import (a, 16, 1, 1, 0, 0, hex2bytes(passphrase.c_str()).data());
	}
	mpz_class A(a);

	// use dbid as exponent "p" number
	mpz_class P(dbid);
	if (verbose_flag)
		std::cout << "P " << P << std::endl;

	// convert n_str into n number
	mpz_class N(n_str);
	if (verbose_flag)
		std::cout << "N " << N << std::endl;

	// compute r = a ^ p mod n
	mpz_class R(0);
	mpz_powm(R.get_mpz_t(), A.get_mpz_t(), P.get_mpz_t(), N.get_mpz_t());
	if (verbose_flag)
		std::cout << "R " << R << std::endl;

	// use username hash as number "u"
	// different password for each user
	Crypto::MD5 U_MD5(username);
	mpz_t u;
	mpz_init(u);
	mpz_import (u, 16, 1, 1, 0, 0, U_MD5.bindigest());
	mpz_class U(u);
	if (verbose_flag)
	{
		std::cout << "U " << U_MD5.hexdigest() << std::endl;
		std::cout << "U " << U << std::endl;
	}

	mpz_class R1 = R;
	mpz_powm(R.get_mpz_t(), R1.get_mpz_t(), U.get_mpz_t(), N.get_mpz_t());

	// just burn some CPU time
	// compute r = a ^ r mod n
	for (int i=0; i<100; i++)
	{
		R1 = R;
		mpz_powm(R.get_mpz_t(), A.get_mpz_t(), R1.get_mpz_t(), N.get_mpz_t());
	}
	if (verbose_flag)
	{
		std::cout << "R " << R << std::endl;
		std::cout << "R1 " << R1 << std::endl;
	}

	// compute sha(r)
	size_t size = (mpz_sizeinbase (R.get_mpz_t(), 2) + CHAR_BIT-1) / CHAR_BIT;
	char *buff = (char*)calloc(1, size);
	mpz_export(buff, &size, 1, 1, 0, 0, R.get_mpz_t());
	if (verbose_flag)
		std::cout << "R_LEN " << size << std::endl;

	std::string v(buff, size);
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

	if (verbose_flag)
		std::cout << "r2 " << r2_m_char << std::endl;
	
	retval = r2_m_char;
	
	return retval;
}

#endif
