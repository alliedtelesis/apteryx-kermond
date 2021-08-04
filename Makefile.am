bin_PROGRAMS = apteryx-kermond

apteryx_kermond_CFLAGS = \
	@CFLAGS@ \
	@LIBNL_CFLAGS@ \
	@LIBNLROUTE_CFLAGS@ \
	@APTERYX_CFLAGS@ \
	@GLIB_CFLAGS@

apteryx_kermond_LDADD = \
	@LIBNL_LIBS@ \
	@LIBNLROUTE_LIBS@ \
	@APTERYX_LIBS@ \
	@GLIB_LIBS@

apteryx_kermond_SOURCES = \
	main.c \
	module.c \
	apteryx.c \
	netlink.c \
	procfs.c \
	interface/ifstatus.c \
	interface/ifconfig.c \
	iprouting/rib.c \
	iprouting/fib.c \
	neighbor/settings.c \
	icmp/icmp.c tcp/tcp.c \
	entity/entity.c \
	ip/address-cache.c \
	ip/address-static.c \
	ip/neighbor-cache.c \
	ip/neighbor-static.c

BUILT_SOURCES = \
	interface/interface.h \
	iprouting/iprouting.h \
	neighbor/ip-neighbor.h \
	icmp/ip-icmp.h \
	tcp/ip-tcp.h \
	entity/entity.h \
	ip/ietf-ip.h

CLEANFILES = $BUILT_SOURCES

XML2C ?= ./xml2c

SUFFIXES = .h .xml .yang

%.h : %.xml
	@echo "Generating "$@""
	$(Q)$(XML2C) $<  > $@

%.xml: %.yang
	@echo "Generating "$@""
	$(Q)pyang -f yin $< | xsltproc apteryx-xml.xslt - > $@


if HAVE_TESTS
noinst_PROGRAMS = unittest

unittest_CFLAGS = \
	$(apteryx_kermond_CFLAGS) \
	-g -fprofile-arcs -fprofile-dir=gcov -ftest-coverage \
	-Wl,--wrap=system \
	-Wl,--wrap=if_nametoindex \
	-Wl,--wrap=if_indextoname \
	-Wl,--wrap=rtnl_link_get_by_name \
	-Wl,--wrap=rtnl_link_change \
	-Wl,--wrap=rtnl_addr_add \
	-Wl,--wrap=rtnl_addr_delete \
	-Wl,--wrap=rtnl_neigh_add \
	-Wl,--wrap=rtnl_neigh_delete \
	-Wl,--wrap=nl_cache_foreach_filter \
	-Wl,--wrap=apteryx_search \
	-Wl,--wrap=apteryx_get_tree \
	-Wl,--wrap=apteryx_set_full \
	-Wl,--wrap=apteryx_set_tree_full \
	-Wl,--wrap=apteryx_prune \
	-Wl,--wrap=procfs_read_uint32 \
	-Wl,--wrap=procfs_read_string

unittest_LDADD = \
	$(apteryx_kermond_LDADD)

unittest_SOURCES = \
	test.c \
	apteryx.c \
	netlink.c \
	entity/test_entity.c \
	icmp/test_icmp.c \
	interface/test_ifconfig.c \
	interface/test_ifstatus.c \
	ip/test_address_cache.c \
	ip/test_address_static.c \
	ip/test_neighbor_cache.c \
	ip/test_neighbor_static.c \
	neighbor/test_settings.c \
	tcp/test_tcp.c

test: unittest
	@echo "Running unit tests"
	./unittest
	@mv -f *.gcno gcov/ 2>/dev/null; true
	@mv -f *.gcda gcov/ 2>/dev/null; true
	@lcov -q --capture --directory . --output-file gcov/coverage.info
	@genhtml -q gcov/coverage.info --output-directory gcov
	@echo "Tests have been run!"
endif
