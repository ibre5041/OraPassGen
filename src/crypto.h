#pragma once

#include <string>

std::string genpasswd(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);

#ifdef USE_BOOT_MULTI
std::string genpasswd2(std::string const& dbid, std::string const& username, std::string const& passphrase, std::string const& n_str);
#endif
