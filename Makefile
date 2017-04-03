# Makefile for apteryx-kermond
ifneq ($(V),1)
	Q=@
endif

SRCDIR ?= .
DESTDIR ?= ./
PREFIX ?= /usr/
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
PKG_CONFIG ?= pkg-config
APTERYX_PATH ?=
XML2C ?= xml2c
LIBNL_PATH ?=
INDENT = VERSION_CONTROL=none indent -npro -gnu -nut -bli0 -c1 -cd1 -cp1 -i4 -l92 -ts4 -nbbo -sc
GREP = grep --line-buffered --color=always

define PASS
,-.----.                                      
\    /  \                                     
|   :    |              .--.--.    .--.--.    
|   | .\ :  ,--.--.    /  /    '  /  /    '   
.   : |: | /       \  |  :  /`./ |  :  /`./   
|   |  \ :.--.  .-. | |  :  ;_   |  :  ;_     
|   : .  | \__\/: . .  \  \    `. \  \    `.  
:     |`-' ," .--.; |   `----.   \ `----.   \ 
:   : :   /  /  ,.  |  /  /`--'  //  /`--'  / 
|   | :  ;  :   .'   \'--'.     /'--'.     /  
`---'.|  |  ,     .-./  `--'---'   `--'---'   
  `---`   `--`---'                            
endef
export PASS
define FAIL
                               ,--,    
  .--.,               ,--,   ,--.'|    
,--.'  \            ,--.'|   |  | :    
|  | /\/            |  |,    :  : '    
:  : :    ,--.--.   `--'_    |  ' |    
:  | |-, /       \  ,' ,'|   '  | |    
|  : :/|.--.  .-. | '  | |   |  | :    
|  |  .' \__\/: . . |  | :   '  : |__  
'  : '   ," .--.; | '  : |__ |  | '.'| 
|  | |  /  /  ,.  | |  | '.'|;  :    ; 
|  : \ ;  :   .'   \;  :    ;|  ,   /  
|  |,' |  ,     .-./|  ,   /  ---`-'   
`--'    `--`---'     ---`-'            
endef
export FAIL
FORMAT_RESULTS = $(GREP) -v "^np: running" | sed -u 's/$(DAEMON).//g' $(COLOR_RESULTS) $(DISPLAY_BANNER)
COLOR_RESULTS = | $(GREP) -E 'FAIL|$$' | GREP_COLOR='01;32' $(GREP) -E 'PASS|$$'
DISPLAY_BANNER = | tee /dev/stderr | grep -q 'run 0 failed' && echo "\033[32m$$PASS\033[m" || echo "\033[31m$$FAIL\033[m"

CFLAGS := $(CFLAGS) -g -O2
EXTRA_CFLAGS += -Werror -Wall -Wno-comment -std=c99 -D_GNU_SOURCE -fPIC
EXTRA_CFLAGS += -I$(SRCDIR) `$(PKG_CONFIG) --cflags glib-2.0`
EXTRA_LDFLAGS += `$(PKG_CONFIG) --libs-only-l glib-2.0`
ifndef LIBNL_PATH
EXTRA_CFLAGS += `$(PKG_CONFIG) --cflags libnl-3.0 libnl-route-3.0`
EXTRA_LDFLAGS+= `$(PKG_CONFIG) --libs-only-l libnl-3.0 libnl-route-3.0`
else
EXTRA_CFLAGS += -I$(LIBNL_PATH)/include
EXTRA_LDFLAGS += -L$(LIBNL_PATH)/lib/.libs/ -lnl-3 -lnl-route-3
endif
ifndef APTERYX_PATH
EXTRA_CFLAGS += $(shell $(PKG_CONFIG) --cflags apteryx)
EXTRA_LDFLAGS += $(shell $(PKG_CONFIG) --libs apteryx)
else
EXTRA_CFLAGS += -I$(APTERYX_PATH)
EXTRA_LDFLAGS += -L$(APTERYX_PATH) -lapteryx
endif
EXTRA_LDFLAGS += -lpthread

NOVAPROVA_CFLAGS= `$(PKG_CONFIG) --cflags novaprova`
NOVAPROVA_LIBS := $(EXTRA_LDFLAGS) `$(PKG_CONFIG) --libs novaprova` -lz -liberty `$(PKG_CONFIG) --libs-only-l libnl-3.0 libnl-route-3.0`
EXTRA_LDFLAGS += -Wl,--no-as-needed

DAEMON = apteryx-kermond

SOURCE := main.c module.c apteryx.c netlink.c
SOURCE += interface/ifstatus.c interface/ifconfig.c
API_XML += interface/interface.xml
TEST_SOURCE += interface/test_ifconfig.c
SOURCE += iprouting/rib.c iprouting/fib.c
API_XML += iprouting/iprouting.xml
SOURCE += neighbor/settings.c neighbor/static.c neighbor/cache.c
API_XML += neighbor/neighbor.xml
TEST_SOURCE += neighbor/test_settings.c
TEST_SOURCE += neighbor/test_static.c
SOURCE += icmp/icmp.c
API_XML += icmp/icmp.xml
TEST_SOURCE += icmp/test_icmp.c
SOURCE += tcp/tcp.c
API_XML += tcp/tcp.xml
TEST_SOURCE += tcp/test_tcp.c

OBJS=$(SOURCE:%.c=%.o)
INCLUDES=$(API_XML:%.xml=%.h)

all: $(INCLUDES) $(DAEMON)

%.h: %.xml
	@echo "Generating "$@""
	$(Q)$(XML2C) $<  > $@

%.o: %.c
	@echo "Compiling "$<""
	$(Q)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

$(DAEMON): $(OBJS) kermond.h
	@echo "Building $@"
	$(Q)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $(OBJS) $(EXTRA_LDFLAGS)

indent:
	@echo "Fixing coding-style..."
	$(Q)$(INDENT) $(SOURCE) $(TEST_SOURCE) kermond.h

ifeq (test, $(firstword $(MAKECMDGOALS)))
ifneq ($(word 2, $(MAKECMDGOALS)),)
TESTSPEC = $(notdir $(shell pwd)).$(word 2, $(MAKECMDGOALS))
endif
endif
ifndef NOVAPROVA_VALGRIND
ifeq ($(VALGRIND),no)
export NOVAPROVA_VALGRIND=no
endif
endif

test: $(INCLUDES) $(TEST_SOURCE)
	@echo "Building $@"
	$(Q)$(CC) -g -fprofile-arcs -ftest-coverage $(EXTRA_CFLAGS) $(NOVAPROVA_CFLAGS) -o $@ netlink.c apteryx.c module.c test.c $(TEST_SOURCE) $(NOVAPROVA_LIBS)
	$(Q)VALGRIND_OPTS=--suppressions=valgrind.supp ./test $(TESTSPEC) 2>&1 | $(FORMAT_RESULTS)

install: all
	@install -d $(DESTDIR)/$(PREFIX)/bin
	@install -D apteryx-kermond $(DESTDIR)/$(PREFIX)/bin/
	@install -d $(DESTDIR)/$(PREFIX)/include/apteryx
	@for i in $(INCLUDES) ; do \
		install -D $$i $(DESTDIR)/$(PREFIX)/include/apteryx/ ; \
	done

clean:
	@echo "Cleaning..."
	$(Q)rm -f $(DAEMON) $(OBJS) $(INCLUDES) test *.gcda *.gcno

.PHONY: all clean test indent
