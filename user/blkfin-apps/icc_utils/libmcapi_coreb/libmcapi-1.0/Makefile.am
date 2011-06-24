AUTOMAKE_OPTIONS = foreign

# INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/../../../../linux-2.6.x/include -I$(top_srcdir)/../../../../linux-2.6.x/drivers/staging/icc/include/ -I$(top_srcdir)/../../../../linux-2.6.x/arch/blackfin/mach-bf561/include/

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/../../../../../linux-2.6.x/drivers/staging/icc/include/
library_includedir = /usr/include/mcapi-1.0_coreb
library_include_HEADERS = include/mcapi.h include/mcapi_datatypes.h  include/mcapi_dev_impl.h include/mcapi_trans.h  include/transport_sm.h
lib_LTLIBRARIES = libmcapi_coreb.la
libmcapi_coreb_la_SOURCES  = mcapi.c mcapi_trans_stub.c


