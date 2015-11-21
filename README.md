
About
=====

OraPassGen is a simple tool which generates deterministic passwords from two inputs.

  - Passphrase (entered by a user)
  - dbid  - unique Oracle database identifier

Usage
=====

This tool consisist of there parts:

  - opassgen - command line tool which prompts for passphrase and queries the database dbid and then sets SYS user password
  - dbpassgui - GUI tool which generates the same passwords as opassgen
  - genn - utility tool for modulo generation

Notes
=====

This is NOT password manager, there is NO password database stored anywhere.

License: BSD
=====
