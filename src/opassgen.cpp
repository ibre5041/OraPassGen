
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <getopt.h>
#include <dlfcn.h>
#ifdef ORACLE_FOUND
#include "trotl.h"
using namespace trotl;
#endif
#else
#include "getopt_long.h"
#endif

#include <string>
#include <vector>
#include <ostream>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "common.h"
#include "crypto.h"
#include "dbutils.h"

using namespace std;

static void usage()
{
	printf(
		"                                      \n"
		"Usage:                                \n"
		"  --dbid        <numeric database id> \n"
		"  --username    <username,username>   \n"
		"  --passphrase  <passphrase>          \n"
		"  --only-password                     \n"
		"  --verbose                           \n"
		"  --apply                             \n"
		"  --create-keyfile                    \n"
		"  --version                           \n"
		"                                      \n"
		);
}

extern "C" {
	extern char _binary_resources_n_txt_start;
	extern char _binary_resources_n_txt_size;
	extern char _binary_resources_n_txt_end;
}

int main(int argc, char *argv[])
{
	string passphrase, dbid;
	vector<string> usernames;
	bool only_password(false);
	bool show(true);
	bool apply(false);
	bool create_keyfile(false);

#ifdef _WIN32
	char *p = strrchr(argv[0], '\\');
#else
	char *p = strrchr(argv[0], '/');
#endif
	if (p)
	{
	    p[0] = 0;
	    chdir(argv[0]);
	}

	while (1)
	{
		static struct option long_options[] = {
			/* These options set a flag. */
			{"verbose", no_argument,       &verbose_flag, 1},
			/* These options don’t set a flag.
			   We distinguish them by their indices. */
			{"dbid",       required_argument, 0, 'I'},
			{"username",   required_argument, 0, 'u' },
			{"passphrase", required_argument, 0, 'P'},
			{"only-password", no_argument,    0, 'o'},
			{ "apply",     no_argument,       0, 'a' },
			{ "create-keyfile", no_argument,  0, 'c' },
			{ "help",      no_argument,       0, 'h'},
			{ "version",   no_argument,       0, 'v'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;
		int c = getopt_long (argc, argv, "oachI:P:u:v", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			break;
		case 'I':
			dbid = optarg;
			break;
		case 'u':
			{
				string u = optarg;
				usernames = split(u, ',');
			}
			break;
		case 'P':
			passphrase = optarg;
			{ // hide passphrase from commandline
				size_t idx = 0;
				while(optarg[idx])
				{
					optarg[idx++] = '*';
				}
			}
			break;
		case 'o':
		    only_password = true;
		    break;
		case 'h':
			usage();
			return 0;
		case '?':
			/* getopt_long already printed an error message. */
			break;
		case 'v':
			printf(version_string().c_str());
			return 0;
		case 'c':
			create_keyfile = true;
			break;
		case 'a':
			apply = true;
			break;
		default:
			abort ();
		}
	}
	if (verbose_flag)
		puts ("verbose flag is set");

	if (passphrase.empty() && !create_keyfile)
	{
		read_key_env(passphrase);
		if (!passphrase.empty())
			verbose_flag = show = false;
	}

	if (passphrase.empty() && !create_keyfile)
	{
		read_keyfile(passphrase);
		if (!passphrase.empty())
			verbose_flag = show = false;
	}

	if (passphrase.empty())
	{
		prompt_passphrase(passphrase);
	}

	if (create_keyfile)
	{
		write_keyfile(passphrase);
		return 0;
	}

#if defined(__unix__) && defined(ORACLE_FOUND)
	if (dbid.empty() || apply)
	try
	{ // connect / as sysdba
		OciEnvAlloc _envalloc;
		OciEnv _env(_envalloc);
		OciLogin _login(_env);
		OciConnection _con(_env, _login);
		SqlStatement q0(_con, "select dbid, name from v$database");
		std::string name, sid;
		while(!q0.eof())
		{
			q0 >> dbid >> name;
			std::cout << " dbid: " << dbid << std::endl;
			std::cout << " name: " << name << std::endl;
		}
		SqlStatement q1(_con, "select instance_name from v$instance");
		while(!q1.eof())
		{
			q1 >> sid;
			std::cout << " sid:  " << sid << std::endl;
		}
	}
	catch (OciException const& e)
	{
		std::cerr << e.what();
		return 1;
	}			
#endif

	string n_str;
#ifdef __linux__
#if 0
	void* elf_handle = dlopen(0,RTLD_NOW|RTLD_GLOBAL); // dlopen self
	// - generate factor file:           genn --decimal -f n.txt
	// - compile n.txt into .elf format: objcopy --input binary --output elf64-x86-64 --binary-architecture i386 resources/n.txt n.o
	// - append n.o src/CMakeLists.txt common_sources variable
	size_t n_len = (size_t) dlsym(elf_handle, "_binary_resources_n_txt_size");
	char *n_data = (char *) dlsym(elf_handle, "_binary_resources_n_txt_start");
	if (n_len)
	{ // 1st check whether n was compiled into this binary		
		n_str = std::string(n_data, n_len);
	}
#endif
	{
		char* s = &_binary_resources_n_txt_start;
		char* e = &_binary_resources_n_txt_end;
		n_str = std::string(s, e-s);
	}
#endif
	if (n_str.empty())
	{ // 2n read n from file		
		n_str = slurp(N_FILE_DEC);
		if (n_str.empty())
		{
			std::cerr << "File not found: " << N_FILE_DEC << std::endl;
			exit(3);
		}
	}
	if (n_str[n_str.size()-1] == '\n')
	{
		n_str = n_str.substr(0, n_str.size()-1); // c++11 n_str.pop_back();
	}
	
	if (usernames.empty())
	{
		usernames.push_back("SYS");
		usernames.push_back("SYSTEM");

		//usernames.push_back("ANONYMOUS");
		//usernames.push_back("APPQOSSYS");
		//usernames.push_back("AUDSYS");
		//usernames.push_back("DBSNMP");
		//usernames.push_back("DIP");
		//usernames.push_back("GSMADMIN_INTERNAL");
		//usernames.push_back("GSMCATUSER");
		//usernames.push_back("GSMUSER");
		//usernames.push_back("OJVMSYS");
		//usernames.push_back("OUTLN");
		//usernames.push_back("SYSBACKUP");
		//usernames.push_back("SYSDG");
		//usernames.push_back("SYSKM");
		//usernames.push_back("WMSYS");
		//usernames.push_back("XDB");
	}

	vector<string> statements;
	vector<string> passwords;

	for (vector<string>::iterator it = usernames.begin(); it != usernames.end(); ++it)
	{
		string gen_password = GENPASSWD(dbid, *it, passphrase, n_str);

		stringstream statement;
		// Max username length is 30 CHARs
		statement << "alter user " << left << setw(30) << *it << " identified by \"" << gen_password << "\"";
		statements.push_back(statement.str());
		passwords.push_back(gen_password);

#if defined(__unix__) && defined(ORACLE_FOUND)
		if (show == false) {
			continue;
		} else if (only_password) {
			cout << gen_password << endl;
		} else {
			cout << " " << statement.str() << ";" << endl;
		}
#endif
	}

#if defined(__unix__) && defined(ORACLE_FOUND)
	if (apply)
	{
		try
		{ // connect / as sysdba
			OciEnvAlloc _envalloc;
			OciEnv _env(_envalloc);
			OciLogin _login(_env);
			OciConnection _con(_env, _login);

			for (vector<string>::iterator it = statements.begin(); it != statements.end(); ++it)
			{
				// try once again, skip missing users
				try {
					SqlStatement q0(_con, *it);
					q0.eof();
					cout << endl << "User altered." << endl;
				}
				catch (OciException const& e)
				{
					std::cerr << e.what();
				}
			}
		}
		catch (OciException const& e)
		{
			std::cerr << e.what();
			return 1;
		}
	}
#else
	for (vector<string>::iterator it = passwords.begin(); it != passwords.end(); ++it)
	{
		cout << *it << endl;
	}
#endif

}
