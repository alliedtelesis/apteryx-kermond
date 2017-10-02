/**
 * @file test_static.c
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
#include "neighbor-static.c"

#include <np.h>

#define IFNAME      "eth1"
#define LLADDR      "00:11:22:33:44:55"
#define ADDRV4      "192.168.1.1"
#define ADDRV6      "fc00::2"
#define PATHV4      INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_NEIGHBOR"/"ADDRV4"/"
#define PATHV6      INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_NEIGHBOR"/"ADDRV6"/"

static bool link_active;
static unsigned int
mock_if_nametoindex(const char *ifname)
{
    return link_active ? 1 : 0;
}

static struct rtnl_neigh *neighbor_added = NULL;
static int
mock_rtnl_neigh_add(struct nl_sock *sk, struct rtnl_neigh *tmpl, int flags)
{
    NP_ASSERT_NULL (neighbor_added);
    neighbor_added = (struct rtnl_neigh *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

static struct rtnl_neigh *neighbor_deleted = NULL;
static int
mock_rtnl_neigh_delete(struct nl_sock *sk, struct rtnl_neigh *tmpl, int flags)
{
    NP_ASSERT_NULL (neighbor_deleted);
    neighbor_deleted = (struct rtnl_neigh *) nl_object_clone ((struct nl_object *) tmpl);
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
assert_neigh_valid (struct rtnl_neigh *n, int family, bool check_lladdr)
{
    struct nl_addr *addr;
    char buf[128];

    NP_ASSERT_EQUAL (rtnl_neigh_get_family (n), family);
    NP_ASSERT_EQUAL (rtnl_neigh_get_state (n), NUD_PERMANENT);
    NP_ASSERT_EQUAL (rtnl_neigh_get_ifindex (n), 1);
    addr = rtnl_neigh_get_dst (n);
    if (family == AF_INET)
        NP_ASSERT_STR_EQUAL (nl_addr2str (addr, buf, sizeof(buf)), ADDRV4);
    else
        NP_ASSERT_STR_EQUAL (nl_addr2str (addr, buf, sizeof(buf)), ADDRV6);
    nl_addr_put (addr);
    if (check_lladdr)
    {
        addr = rtnl_neigh_get_lladdr (n);
        NP_ASSERT_STR_EQUAL (nl_addr2str (addr, buf, sizeof(buf)), LLADDR);
        nl_addr_put (addr);
    }
}

static void
setup_test (bool active, char *ignore)
{
    link_active = active;
    np_mock (rtnl_neigh_add, mock_rtnl_neigh_add);
    np_mock (rtnl_neigh_delete, mock_rtnl_neigh_delete);
    np_mock (if_nametoindex, mock_if_nametoindex);
    np_mock (apteryx_search, mock_apteryx_search);
    np_mock (apteryx_get_tree, mock_apteryx_get_tree);
    if (ignore)
        np_syslog_ignore (ignore);
}

/* IPv4 Static Neighbors */

void test_static_neighbor4_path_null ()
{
    setup_test (true, "Invalid static neighbor");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (NULL, "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor4_path_invalid ()
{
    setup_test (true, "Invalid static neighbor");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV4 "dog", "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor4_lladdr_invalid ()
{
    setup_test (true, "Unable to parse phys-address");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV4
            INTERFACES_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor4_ip_invalid ()
{
    setup_test (true, "Unable to parse ip");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_NEIGHBOR"/999.1_/"
            INTERFACES_IPV4_NEIGHBOR_IP, "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor4_add_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV4
            INTERFACES_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, LLADDR));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor4_add()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV4
            INTERFACES_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, LLADDR));
    NP_ASSERT_NOT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
    assert_neigh_valid (neighbor_added, AF_INET, true);
}

void test_static_neighbor4_add_interface_go_active ()
{
    setup_test (true, NULL);
    search_result = g_list_append (search_result,
            strdup (INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_NEIGHBOR"/"ADDRV4));
    apteryx_tree = g_node_new (strdup (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_NEIGHBOR"/"ADDRV4));
    APTERYX_LEAF (apteryx_tree,
            strdup (INTERFACES_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS), strdup (LLADDR));
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NOT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
    assert_neigh_valid (neighbor_added, AF_INET, true);
}

void test_static_neighbor4_delete_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV4
            INTERFACES_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, NULL));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor4_delete ()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV4
            INTERFACES_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, NULL));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NOT_NULL (neighbor_deleted);
    assert_neigh_valid (neighbor_deleted, AF_INET, false);
}

/* IPv6 Static Neighbors */

void test_static_neighbor6_path_null ()
{
    setup_test (true, "Invalid static neighbor");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (NULL, "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor6_path_invalid ()
{
    setup_test (true, "Unexpected static neighbor parameter");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV6 "dog", "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor6_lladdr_invalid ()
{
    setup_test (true, "Unable to parse phys-address");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV6
            INTERFACES_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor6_ip_invalid ()
{
    setup_test (true, "Unable to parse ip");
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_NEIGHBOR"/999.1_/"
            INTERFACES_IPV6_NEIGHBOR_IP, "1"));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor6_add_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV6
            INTERFACES_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, LLADDR));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor6_add ()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV6
            INTERFACES_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, LLADDR));
    NP_ASSERT_NOT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
    assert_neigh_valid (neighbor_added, AF_INET6, true);
}

void test_static_neighbor6_add_interface_go_active ()
{
    setup_test (true, NULL);
    search_result = g_list_append (search_result,
            strdup (INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_NEIGHBOR"/"ADDRV6));
    apteryx_tree = g_node_new (strdup (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_NEIGHBOR"/"ADDRV6));
    APTERYX_LEAF (apteryx_tree,
            strdup (INTERFACES_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS), strdup (LLADDR));
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NOT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
    assert_neigh_valid (neighbor_added, AF_INET6, true);
}

void test_static_neighbor6_delete_interface_inactive ()
{
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV6
            INTERFACES_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, NULL));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NULL (neighbor_deleted);
}

void test_static_neighbor6_delete ()
{
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_neighbors_cb (PATHV6
            INTERFACES_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, NULL));
    NP_ASSERT_NULL (neighbor_added);
    NP_ASSERT_NOT_NULL (neighbor_deleted);
    assert_neigh_valid (neighbor_deleted, AF_INET6, false);
}
