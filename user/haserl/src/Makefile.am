PACKAGE=@PACKAGE_TARNAME@

datarootdir=@datarootdir@

if MAKELUA
LUASOURCE = h_lua.h h_lua.c h_luascript.c
endif

bin_PROGRAMS = haserl 

haserl_SOURCES = common.c common.h \
		 h_error.c h_error.h h_script.c h_script.h h_bash.c h_bash.h \
		 $(LUASOURCE) haserl.c haserl.h

install-strip:
        $(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
          install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
          `test -z '$(STRIP)' || \
            echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
