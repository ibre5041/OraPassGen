#pragma once

#include <string>

#ifdef USE_OPENSSL
std::string genpasswd_openssl(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#endif

#ifdef USE_BOOST_MULTIPRECISION
std::string genpasswd_boost(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#endif

#if 1
std::string genpasswd_mpir(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#endif

#define GENPASSWD genpasswd_mpir


