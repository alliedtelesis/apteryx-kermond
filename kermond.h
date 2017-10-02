/**
 * @file kermond.h
 * Internal header for apteryx-kermond.
 *
 * Copyright 2017, Allied Telesis Labs New Zealand, Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>
 */
#ifndef _KERMOND_H_
#define _KERMOND_H_
#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/time.h>
#include <syslog.h>
#include <glib.h>
#include <glib-unix.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <linux/if.h>
#include <apteryx.h>

/* GLib Main Loop */
extern GMainLoop *g_loop;

/* Debug */
extern bool kermond_debug;
extern bool kermond_verbose;
#define VERBOSE(fmt, args...) if (kermond_verbose) printf (fmt, ## args)
#define DEBUG(fmt, args...) if (kermond_debug) printf (fmt, ## args)
#define INFO(fmt, args...) { if (kermond_debug) printf (fmt, ## args); else syslog (LOG_INFO, fmt, ## args); }
#define NOTICE(fmt, args...) { if (kermond_debug) printf (fmt, ## args); else syslog (LOG_NOTICE, fmt, ## args); }
#define ERROR(fmt, args...) { if (kermond_debug) printf (fmt, ## args); else syslog (LOG_CRIT, fmt, ## args); }
#define FATAL(fmt, args...) { if (kermond_debug) printf (fmt, ## args); else syslog (LOG_CRIT, fmt, ## args); g_main_loop_quit (g_loop); }
#define ASSERT(assertion, rcode, fmt, args...) { \
    if (!(assertion)) { \
        if (kermond_debug) printf (fmt, ## args); \
        else syslog (LOG_CRIT, fmt, ## args); \
        g_main_loop_quit (g_loop); \
        rcode; \
    } \
}

/* Utilities */
#define xstr(a) str(a)
#define str(a) #a

/* Modules */
typedef struct module_table
{
    bool enabled;
    const char name[64];
      bool (*init) (void);
      bool (*start) (void);
    void (*exit) (void);
} module_table;

#define MODULE_ATTRIBUTES __attribute__((used)) __attribute__ ((aligned (8)))
#define MODULE_CREATE(name, init, start, exit) \
    static const module_table module_entry \
    MODULE_ATTRIBUTES __attribute__((__section__("module_table"))) = { true, name, init, start, exit }
bool modules_enable (const char *modules);
void modules_dump (void);
bool modules_init (void);
bool modules_start (void);
void modules_exit (void);

/* Apteryx helpers */
void apteryx_rewatch_tree (char *path, apteryx_watch_callback cb);
bool apteryx_parse_boolean (const char *path, const char *value, bool defvalue);

/* Netlink functions */
#if LIBNL_VER_NUM < LIBNL_VER(3,2) || (LIBNL_VER_NUM == LIBNL_VER(3,2) && LIBNL_VER_MIC < 27)
enum
{
    NL_ACT_UNSPEC,
    NL_ACT_NEW,
    NL_ACT_DEL,
    NL_ACT_GET,
    NL_ACT_SET,
    NL_ACT_CHANGE,
    __NL_ACT_MAX,
};
#endif
extern struct nl_dump_params netlink_dp;
typedef void (*netlink_callback) (int action, struct nl_object * old_obj,
                                  struct nl_object * new_obj);
bool netlink_init ();
void netlink_exit ();
bool netlink_register (char *kind, netlink_callback cb);
void netlink_unregister (char *kind, netlink_callback cb);

/* ProcFS functions */
uint32_t procfs_read_uint32 (const char *path);
char* procfs_read_string (const char *path);
void procfs_write_uint32 (const char *path, uint32_t value);

#endif /* _KERMOND_H_ */
