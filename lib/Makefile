##############################################################################
#
#	Makefile -- Master makefile for all libraries.
#

.EXPORT_ALL_VARIABLES:

##############################################################################
#
# Include architecture specific build rules.
#

ifndef ROOTDIR
ROOTDIR=$(PWD)/..
endif

UCLINUX_BUILD_LIB=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

##############################################################################
#
# always build these dir_
#
dir_1_y  = $(LIBCDIR)
dir_1_n  =
dir_1_   =

ifeq ($(LIBCDIR),libc)
dir_2_y += libm libcrypt_old
dir_2_n +=
dir_2_  +=
endif

#
# and build these if someone needs them
#
dir_3_$(CONFIG_LIB_FLEX)         += flex
dir_3_$(CONFIG_LIB_LIBAES)       += libaes
dir_3_$(CONFIG_LIB_LIBDES)       += libdes
dir_3_$(CONFIG_LIB_LIBSSL)       += libssl
dir_3_$(CONFIG_LIB_LIBGMP)       += libgmp
dir_3_$(CONFIG_LIB_LIBG)         += libg
dir_3_$(CONFIG_LIB_LIBPAM)       += libpam
dir_3_$(CONFIG_LIB_LIBPCAP)      += libpcap
dir_3_$(CONFIG_LIB_ZLIB)         += zlib
dir_3_$(CONFIG_LIB_LIBBZ2)       += libbzip2
dir_3_$(CONFIG_LIB_LIBATM)       += libatm
dir_3_$(CONFIG_LIB_LIBNET)       += Libnet
dir_3_$(CONFIG_LIB_LIBNETOLD)    += libnet
dir_3_$(CONFIG_LIB_LIBJPEG)      += libjpeg
dir_3_$(CONFIG_LIB_LIBUPNP)      += libupnp
dir_3_$(CONFIG_LIB_STLPORT)      += STLport
dir_3_$(CONFIG_LIB_EXPAT)        += expat
dir_3_$(CONFIG_LIB_LIBLDAP)      += libldap
dir_3_$(CONFIG_LIB_TINYTCL)      += ../user/tinytcl
dir_3_$(CONFIG_LIB_LIBCCMALLOC)  += libccmalloc
dir_3_$(CONFIG_LIB_ARES)         += libares
dir_3_$(CONFIG_LIB_LIBIDN)       += libidn
dir_3_$(CONFIG_LIB_LIBCURL)      += libcurl
dir_3_$(CONFIG_LIB_LIBCRYPT_OLD) += libcrypt_old
dir_3_$(CONFIG_LIB_LIBLZO)       += lzo
dir_3_$(CONFIG_LIB_LIBPCRE)      += pcre
dir_3_$(CONFIG_LIB_LIBLZMA)      += liblzma
dir_3_$(CONFIG_LIB_OSIP2)        += osip2
dir_3_$(CONFIG_LIB_LIBEFENCE)    += libefence
dir_3_$(CONFIG_LIB_TERMCAP)      += termcap
dir_3_$(CONFIG_LIB_NCURSES)      += ncurses
dir_3_$(CONFIG_LIB_LIBSYSFS)     += sysfsutils
dir_3_$(CONFIG_LIB_LIBNL)        += libnl
dir_3_$(CONFIG_LIB_NFNETLINK)    += libnfnetlink
dir_3_$(CONFIG_USER_TCL_LIBTCL)  += ../user/tcl
dir_3_$(CONFIG_LIB_LIBIBERTY)    += libiberty
dir_3_$(CONFIG_LIB_BFD)          += bfd
dir_3_$(CONFIG_LIB_POPT)         += popt
dir_3_$(CONFIG_LIB_LIBMPEG2)     += mpeg2dec
dir_3_$(CONFIG_LIB_TSLIB)        += tslib-1.0
dir_3_$(CONFIG_LIB_LIBSDL)       += libsdl
dir_4_$(CONFIG_LIB_FREETYPE)     += freetype        # requires zlib(3)
dir_4_$(CONFIG_LIB_FLTK2)	 += fltk
dir_4_$(CONFIG_LIB_PWLIB)        += pwlib
dir_4_$(CONFIG_LIB_NETFILTER_LOG)+= libnetfilter_log
dir_4_$(CONFIG_LIB_NETFILTER_CONNTRACK) += libnetfilter_conntrack
dir_4_$(CONFIG_LIB_LIBPNG)       += libpng
dir_4_$(CONFIG_LIB_SDL_GFX)      += SDL_gfx         # requires libsdl(3)
dir_5_$(CONFIG_LIB_DIRECTFB)     += DirectFB        # requires zlib(3) libsdl(3) libpng(4)
dir_5_$(CONFIG_LIB_SDL_IMAGE)    += SDL_image       # requires libsdl(3) libpng(4) libjpeg(3) zlib (3) tiff(?)
dir_5_$(CONFIG_LIB_SDL_MIXER)    += SDL_mixer       # requires libsdl(3) tremor(3) SMPEG(?) libmad(3)
dir_5_$(CONFIG_LIB_SDL_NET)      += SDL_net         # requires libsdl(3)
dir_5_$(CONFIG_LIB_SDL_TTF)      += SDL_ttf         # requires libsdl(3) freetype(4)
dir_5_$(CONFIG_LIB_LIBOPENH323)  += libopenh323
dir_5_$(CONFIG_LIB_LIBJUDY)      += libjudy

#
# dir 6, 7 and 8 reserved for Makefile.local
#
-include Makefile.local

# And build libraries in the prop directory last
dir_9_y += $(wildcard $(ROOTDIR)/prop)

DIRS_NUM = 1 2 3 4 5 6 7 8 9
DIRS_y   = $(foreach n,$(DIRS_NUM),$(sort $(dir_$(n)_y)))
DIRS_all = $(foreach n,$(DIRS_NUM),$(sort $(dir_$(n)_y) $(dir_$(n)_n) $(dir_$(n)_)))

##############################################################################

SET_STAGEDIR = \
	(cd $(STAGEDIR); \
	chmod 755 ./usr/lib/lib*.so*; \
	chmod 644 ./usr/lib/lib*.la ./usr/lib/lib*.a; \
	sed -i -e "/^libdir=/s:=.*:='$(STAGEDIR)/usr/lib':" -e "/^dependency_libs=/s: /usr/lib/: $(STAGEDIR)/usr/lib/:g" ./usr/lib/*.la; \
	sed -i "/^prefix=/s:=.*:='$(STAGEDIR)/usr':" ./usr/lib/pkgconfig/*.pc ./usr/bin/*-config; \
	true)

all:
	[ -z "$(dir_1_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_1_y)) || exit $$?
	[ -z "$(dir_1_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_2_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_2_y)) || exit $$?
	[ -z "$(dir_2_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_3_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_3_y)) || exit $$?
	[ -z "$(dir_3_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_4_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_4_y)) || exit $$?
	[ -z "$(dir_4_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_5_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_5_y)) || exit $$?
	[ -z "$(dir_5_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_6_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_6_y)) || exit $$?
	[ -z "$(dir_6_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_7_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_7_y)) || exit $$?
	[ -z "$(dir_7_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_8_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_8_y)) || exit $$?
	[ -z "$(dir_8_y)" ] || $(SET_STAGEDIR)
	[ -z "$(dir_9_y)" ] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_9_y)) || exit $$?
	[ -z "$(dir_9_y)" ] || $(SET_STAGEDIR)

# the actual dir target
.PHONY: $(DIRS_y)
$(DIRS_y):
	[ ! -d "$@" ] || ( touch $@/.sgbuilt_lib && $(MAKE) -C $@ ) || exit $$?

%_only:
	touch $(@:_only=)/.sgbuilt_lib && $(MAKE) -C $(@:_only=) -j$(HOST_NCPU)

%_clean:
	$(MAKE) -C $(@:_clean=) clean; rm -f $(@:_clean=)/.sgbuilt_lib; true

%_romfs:
	$(MAKE) -j1 -C $(@:_romfs=) romfs

##############################################################################

romfs:
	for i in $(DIRS_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done

##############################################################################

clean:
	-for i in $(DIRS_all); do \
		if [ -f $$i/.sgbuilt_lib ]; then \
			$(MAKE) -C $$i clean ; \
			rm -f $$i/.sgbuilt_lib; \
		fi; \
	done

##############################################################################

prune:
	-for i in $(DIRS_all) ; do \
		found=0; \
		for j in $(sort $(DIRS_y)) ; do \
			if [ $$i = $$j ]; then \
				found=1; \
			fi; \
		done; \
		if [ $$found = 0 ]; then \
			rm -fr $$i; \
		fi; \
	done
##############################################################################
