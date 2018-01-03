#pragma once

#include <string>

void read_key_env(std::string &passphrase);

#ifdef BOOST_FOUND
std::string genpasswd_boost(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
void write_keyfile_boost(std::string const&);
void read_keyfile_boost(std::string &);

#define GENPASSWD genpasswd_boost
#define WRITE_KEYFILE write_keyfile_boost
#define READ_KEYFILE read_keyfile_boost
#endif

#ifdef OPENSSL_FOUND
std::string genpasswd_openssl(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
void write_keyfile_openssl(std::string const&);
void read_keyfile_openssl(std::string &);

#undef GENPASSWD
#undef WRITE_KEYFILE
#undef READ_KEYFILE
#define GENPASSWD genpasswd_openssl
#define WRITE_KEYFILE write_keyfile_openssl
#define READ_KEYFILE read_keyfile_openssl
#endif

#ifdef GMP_FOUND
std::string genpasswd_mpir(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
void write_keyfile_mpir(std::string const&);
void read_keyfile_mpir(std::string &);

#undef GENPASSWD
#undef WRITE_KEYFILE
#define GENPASSWD genpasswd_mpir
#define WRITE_KEYFILE write_keyfile_mpir
#define READ_KEYFILE read_keyfile_mpir
#endif




