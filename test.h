/**
 * @file test.h
 * Internal header for unittests
 *
 * Copyright 2021, Allied Telesis Labs New Zealand, Ltd
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
#ifndef _TEST_H_
#define _TEST_H_

#define np_mock(a,b)
#define np_syslog_ignore(a)
#define NP_ASSERT g_assert
#define NP_ASSERT_TRUE g_assert_true
#define NP_ASSERT_FALSE g_assert_false
#define NP_ASSERT_NULL g_assert_null
#define NP_ASSERT_NOT_NULL g_assert_nonnull
#define NP_ASSERT_EQUAL(a,b) g_assert_cmpint(a,==,b)
#define NP_ASSERT_STR_EQUAL(a,b) g_assert_cmpstr(a,==,b)

#define NP_TEST_START \
    if (g_test_subprocess ()) {

#define NP_TEST_END(stdout) \
       exit (0); \
    } \
    g_test_trap_subprocess (NULL, 0, 0); \
    g_test_trap_assert_stdout (stdout); \
    g_test_trap_assert_stderr ("");

#define G_SUBPROCESS(action) \
{ \
    if (g_test_subprocess ()) { \
       action ; \
       exit (0); \
    } \
    g_test_trap_subprocess (NULL, 0, 0); \
}

#define G_SUBPROCESS_E(stdout,action) \
{ \
    if (g_test_subprocess ()) { \
       { action } \
       exit (0); \
    } \
    g_test_trap_subprocess (NULL, 0, 0); \
    g_test_trap_assert_stdout (stdout); \
    g_test_trap_assert_stderr (""); \
}

#define IFNAME  "eth99"
#define IFINDEX 100
#define SIFINDEX "100"
#define IP4ADDR "192.168.1.1"
#define IP6ADDR "fc00::1"
#define LLADDR "00:11:22:33:44:55"
#define ADDRV4 IP4ADDR
#define ADDRV6 IP6ADDR
extern GList *cmds;
extern bool link_active;
extern int addr_family;
extern struct rtnl_link *link_changes;
extern struct rtnl_addr *address_added;
extern struct rtnl_addr *address_deleted;
extern struct rtnl_neigh *neighbor_added;
extern struct rtnl_neigh *neighbor_deleted;
extern GList *search_result;
extern GNode *apteryx_tree;
extern char *apteryx_path;
extern char *apteryx_value;
extern char *apteryx_prune_path;
extern uint32_t procfs_uint32_t;
extern char *procfs_string;

#endif /* _TEST_H_ */
