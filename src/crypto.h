#pragma once

#include <string>

void read_key_env(std::string &passphrase);
void write_keyfile(std::string const& passphrase);
void read_keyfile(std::string &passphrase);

#ifdef BOOST_FOUND
std::string genpasswd_boost(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#define GENPASSWD genpasswd_boost
#endif

#ifdef OPENSSL_FOUND
std::string genpasswd_openssl(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#undef  GENPASSWD
#define GENPASSWD genpasswd_openssl
#endif

#ifdef MPIR_FOUND
std::string genpasswd_mpir(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#undef  GENPASSWD
#define GENPASSWD genpasswd_mpir
#endif
