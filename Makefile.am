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
	src/mod_bonding.c \
	src/mod_bridges.c \
	src/mod_interface.c \
	src/mod_route.c \
	src/mod_entity.c

BUILT_SOURCES = \
	models/at-bonding.h \
	models/at-bonding.xml \
	models/at-bridges.h \
	models/at-bridges.xml \
	models/at-interface.h \
	models/at-interface.xml \
	models/at-route.h \
	models/at-route.xml \
	models/at-entity.h \
	models/at-entity.xml

CLEANFILES = $BUILT_SOURCES

SUFFIXES = .h .yang
CUR_DIR = $(CURDIR)

models/%.h: models/%.yang
	@echo "Generating "$@""
	$(Q)$(PYANG) -f cpaths -o $@ $<

models/%.xml: models/%.yang
	@echo "Generating "$@""
	$(Q)$(PYANG) -f apteryx-xml -o $@ $<
