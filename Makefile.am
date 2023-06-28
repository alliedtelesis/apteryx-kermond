bin_PROGRAMS = apteryx-kermond

apteryx_kermond_CFLAGS = \
	@CFLAGS@ \
	@LIBNL_CFLAGS@ \
	@LIBNLROUTE_CFLAGS@ \
	@APTERYX_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-Isrc \
	-Imodels

apteryx_kermond_LDADD = \
	@LIBNL_LIBS@ \
	@LIBNLROUTE_LIBS@ \
	@APTERYX_LIBS@ \
	@GLIB_LIBS@

apteryx_kermond_SOURCES = \
	src/main.c \
	src/module.c \
	src/apteryx.c \
	src/netlink.c \
	src/procfs.c \
	src/interface/ifstatus.c \
	src/interface/ifconfig.c \
	src/iprouting/rib.c \
	src/iprouting/fib.c \
	src/neighbor/settings.c \
	src/icmp/icmp.c \
	src/tcp/tcp.c \
	src/entity/entity.c \
	src/ip/address-cache.c \
	src/ip/address-static.c \
	src/ip/neighbor-cache.c \
	src/ip/neighbor-static.c

BUILT_SOURCES = \
	models/interface.h \
	models/interface.xml \
	models/iprouting.h \
	models/iprouting.xml \
	models/ip-neighbor.h \
	models/ip-neighbor.xml \
	models/ip-icmp.h \
	models/ip-icmp.xml \
	models/ip-tcp.h \
	models/ip-tcp.xml \
	models/entity.h \
	models/entity.xml \
	models/ietf-interfaces.h \
	models/ietf-interfaces.xml \
	models/ietf-ip.h \
	models/ietf-ip.xml

CLEANFILES = $BUILT_SOURCES

SUFFIXES = .h .yang
CUR_DIR = $(CURDIR)

models/%.h: models/%.yang
	@echo "Generating "$@""
	$(Q)$(PYANG) -f cpaths -o $@ $<

models/%.xml: models/%.yang
	@echo "Generating "$@""
	$(Q)$(PYANG) -f apteryx-xml -o $@ $<
