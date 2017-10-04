/**
 * @file test_address_static.c
 * Unit tests for static neighbors
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
#include "address-static.c"

#include <np.h>

#define IFNAME      "eth1"
#define LLADDR      "00:11:22:33:44:55"
#define ADDRV4      "192.168.1.1"
#define ADDRV6      "fc00::2"
#define PATHV4      INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_ADDRESS"/"ADDRV4"/"
#define PATHV6      INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_ADDRESS"/"ADDRV6"/"

static bool link_active;
static unsigned int
mock_if_nametoindex(const char *ifname)
{
    return link_active ? 1 : 0;
}

static struct rtnl_addr *address_added = NULL;
static int
mock_rtnl_addr_add(struct nl_sock *sk, struct rtnl_addr *tmpl, int flags)
{
    NP_ASSERT_NULL (address_added);
    address_added = (struct rtnl_addr *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

static struct rtnl_addr *address_deleted = NULL;
static int
mock_rtnl_addr_delete(struct nl_sock *sk, struct rtnl_addr *tmpl, int flags)
{
    NP_ASSERT_NULL (address_deleted);
    address_deleted = (struct rtnl_addr *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

static GList *search_result = NULL;
static GList *
mock_apteryx_search (const char *path)
{
    GList *result = search_result;
    NP_ASSERT_NOT_NULL (result);
    search_result = NULL;
    return result;
}

static GNode *apteryx_tree;
static GNode *
mock_apteryx_get_tree (const char *path)
{
    GNode *tree = apteryx_tree;
    NP_ASSERT_NOT_NULL (tree);
    apteryx_tree = NULL;
    return tree;
}

static void
assert_address_valid (struct rtnl_addr *n, int family)
{
    struct nl_addr *addr;
    char buf[128];

    NP_ASSERT_EQUAL (rtnl_addr_get_family (n), family);
    addr = rtnl_addr_get_local (n);
    if (family == AF_INET)
        NP_ASSERT_STR_EQUAL (nl_addr2str (addr, buf, sizeof(buf)), ADDRV4);
    else
        NP_ASSERT_STR_EQUAL (nl_addr2str (addr, buf, sizeof(buf)), ADDRV6);
    nl_addr_put (addr);
}

static void
setup_test (bool active, char *ignore)
{
    link_active = active;
    np_mock (rtnl_addr_add, mock_rtnl_addr_add);
    np_mock (rtnl_addr_delete, mock_rtnl_addr_delete);
    np_mock (if_nametoindex, mock_if_nametoindex);
    np_mock (apteryx_search, mock_apteryx_search);
    np_mock (apteryx_get_tree, mock_apteryx_get_tree);
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_static_addr4_path_null ()
{
    setup_test (true, "Invalid static address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (NULL, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr4_path_invalid ()
{
    setup_test (true, "Invalid static address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4 "dog", "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr4_ip_invalid ()
{
    setup_test (true, "Unable to parse ip");
    NP_ASSERT_TRUE (apteryx_static_address_cb (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_ADDRESS"/999.1_/"
            INTERFACES_STATE_IPV4_ADDRESS_IP, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr4_prefixlen_invalid ()
{
    setup_test (true, "Unable to parse phys-address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_SUBNET_PREFIX_LENGTH, "9999999"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr4_add_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, LLADDR));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr4_add()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, LLADDR));
    NP_ASSERT_NOT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    assert_address_valid (address_added, AF_INET);
}

void test_static_addr4_add_interface_go_active ()
{
    setup_test (true, NULL);
    search_result = g_list_append (search_result,
            strdup (INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_ADDRESS"/"ADDRV4));
    apteryx_tree = g_node_new (strdup (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_ADDRESS"/"ADDRV4));
    APTERYX_LEAF (apteryx_tree,
            strdup (INTERFACES_STATE_IPV4_ADDRESS_IP), strdup (LLADDR));
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NOT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    assert_address_valid (address_added, AF_INET);
}

void test_static_addr4_delete_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr4_delete ()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NOT_NULL (address_deleted);
    assert_address_valid (address_deleted, AF_INET);
}

void test_static_addr6_path_null ()
{
    setup_test (true, "Invalid static address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (NULL, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr6_path_invalid ()
{
    setup_test (true, "Unexpected static address parameter");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6 "dog", "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr6_ip_invalid ()
{
    setup_test (true, "Unable to parse ip");
    NP_ASSERT_TRUE (apteryx_static_address_cb (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_ADDRESS"/999.1_/"
            INTERFACES_IPV6_ADDRESS_IP, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr6_prefixlen_invalid ()
{
    setup_test (true, "Unable to parse phys-address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_PREFIX_LENGTH, "9999999"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr6_add_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, ADDRV6));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr6_add ()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, ADDRV6));
    NP_ASSERT_NOT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    assert_address_valid (address_added, AF_INET6);
}

void test_static_addr6_add_interface_go_active ()
{
    setup_test (true, NULL);
    search_result = g_list_append (search_result,
            strdup (INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_ADDRESS"/"ADDRV6));
    apteryx_tree = g_node_new (strdup (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_ADDRESS"/"ADDRV6));
    APTERYX_LEAF (apteryx_tree,
            strdup (INTERFACES_IPV6_ADDRESS_IP), strdup (ADDRV6));
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NOT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    assert_address_valid (address_added, AF_INET6);
}

void test_static_addr6_delete_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
}

void test_static_addr6_delete ()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NOT_NULL (address_deleted);
    assert_address_valid (address_deleted, AF_INET6);
}
