#pragma once

#include <string>

#ifdef BOOST_FOUND
std::string genpasswd_boost(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#define GENPASSWD genpasswd_boost
#endif

#ifdef OPENSSL_FOUND
std::string genpasswd_openssl(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#undef GENPASSWD
#define GENPASSWD genpasswd_openssl
#endif

#ifdef GMP_FOUND
std::string genpasswd_mpir(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#undef GENPASSWD
#define GENPASSWD genpasswd_mpir
#endif




