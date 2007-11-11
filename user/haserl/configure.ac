# Process this file with autoconf to produce a configure script.
AC_INIT([haserl],[0.9.16],[Nathan Angelacos <nangel@users.sourceforge.net>],[haserl])
AM_INIT_AUTOMAKE([haserl],[$PACKAGE_VERSION])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AM_CONFIG_HEADER(src/config.h)
AC_CHECK_HEADERS([stdlib.h string.h unistd.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset putenv strcasecmp strdup strncasecmp strstr])

AC_DEFINE([_GNU_SOURCE], [], [Enable GNU Extensions])


dnl **************************************************************
AC_ARG_WITH(lua-headers,
   [  --with-lua-headers=DIR  lua include files location],
   [LUA_HDR_DIR="$withval"]
   [CPPFLAGS="$CPPFLAGS -I$withval"] 
)


dnl Checks for lua
AC_ARG_WITH(lua, 
[  --with-lua[[=DIR]]      use lua in DIR],
[ case "$withval" in
  no)
	ac_report_have_lua=disabled
	AM_CONDITIONAL(MAKELUA,false)
	;;	
  *)
	AC_SEARCH_LIBS(dlopen, dl)
	LIBS="-llua -lm $LIBS"   
	LDFLAGS="$LDFLAGS -Wl,-E -L$withval"
	CPPFLAGS="$CPPFLAGS -I$withval"
	ac_report_have_lua=enabled
	AM_CONDITIONAL(MAKELUA, true )
	AC_DEFINE([USE_LUA], [], [Enable Lua])
	;;
  esac ],
 	[  ac_report_have_lua=disabled
	AM_CONDITIONAL(MAKELUA,false)
	]
)

# What prefix should we use
AC_ARG_ENABLE(variable-prefix,
	[  --enable-variable-prefix=prefix - enable prefix to add to all user-supplied variables
		  (default is FORM_)])

# Handle "--disable-variable-prefix"
case $enable_variable_prefix in 
	no )	enable_variable_prefix=
		;;
	"" )	enable_variable_prefix="FORM_"
		;;
	esac

AC_DEFINE_UNQUOTED( [HASERL_VAR_PREFIX], "$enable_variable_prefix", 
	[prefix for user supplied data])

# What subshell should we use
AC_ARG_ENABLE(subshell,
	[  --enable-subshell=cmd - enable the subshell to use (default is /bin/sh)])

# Handle "--disable-subshell"
case $enable_subshell in
	no )	enable_subshell=
		;;
	"" )	enable_subshell="/bin/sh"
		;;
	esac

AC_DEFINE_UNQUOTED( [SUBSHELL_CMD], "$enable_subshell", 
	[the subshell to start up])

CFLAGS="$CFLAGS -Wall"

AC_CONFIG_FILES([Makefile
		src/Makefile
		doc/Makefile
		])




AC_OUTPUT 

dnl  report configuration
AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

	Buidling haserl with with lua $ac_report_have_lua 
	libs: $LIBS
])


