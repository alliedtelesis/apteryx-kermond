/**
 * @file test_neighbor_cache.c
 * Unit tests for IP neighbor cache
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
#include "neighbor-cache.c"

#include <np.h>

#define IFNAME          "eth1"
#define IFINDEX         "99"
#define DST_ADDRESS     "192.168.1.1"
#define DST6_ADDRESS    "fc00::1"
#define LL_ADDRESS      "00:11:22:33:44:55"
#define IF_PATH         INTERFACES_STATE_PATH"/"IFNAME
#define IPv4_PATH       IF_PATH"/"INTERFACES_STATE_IPV4_NEIGHBOR"/"DST_ADDRESS
#define IPv6_PATH       IF_PATH"/"INTERFACES_STATE_IPV6_NEIGHBOR"/"DST6_ADDRESS

static char *
mock_if_indextoname (unsigned int ifindex, char *ifname)
{
    strcpy (ifname, IFNAME);
    return ifname;
}

static gpointer
apteryx_node_copy_fn (gconstpointer src, gpointer data)
{
    return (gpointer) g_strdup ((const gchar *) src);
}

static GNode *apteryx_tree;
static bool
mock_apteryx_set_tree_full (GNode *root, uint64_t ts, bool wait_for_completion)
{
    NP_ASSERT_NULL (apteryx_tree);
    apteryx_tree = g_node_copy_deep (root, apteryx_node_copy_fn, NULL);
    return true;
}

static char *apteryx_path;
static char *apteryx_value;
static bool
mock_apteryx_set_full (const char *path, const char *value, uint64_t ts,
                       bool wait_for_completion)
{
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    apteryx_path = g_strdup (path);
    apteryx_value = g_strdup (value);
    return true;
}

static char *apteryx_prune_path;
static bool
mock_apteryx_prune_path (const char *path)
{
    NP_ASSERT_NULL (apteryx_prune_path);
    apteryx_prune_path = g_strdup (path);
    return true;
}

static GNode*
apteryx_get_node (GNode *tree, char *path)
{
    if (strcmp (path, APTERYX_NAME (tree)) == 0)
        return tree;
    return NULL;
}

static bool
check_tree_parameter (GNode *tree, char *path, char *parameter, char *value)
{
    GNode *node = apteryx_get_node (tree, path);
    node = apteryx_find_child (node, parameter);
    if (!value && node)
        return false;
    if (value && node && strcmp (APTERYX_VALUE (node), value) != 0)
        return false;
    return true;
}

static struct nl_object *
make_neighbor (int family, char *dst, char *lladdr)
{
    struct rtnl_neigh *rn = rtnl_neigh_alloc ();
    rtnl_neigh_set_ifindex (rn, atoi (IFINDEX));
    rtnl_neigh_set_family (rn, family);
    if (dst)
    {
        struct nl_addr *addr;
        nl_addr_parse (dst, family, &addr);
        NP_ASSERT_NOT_NULL (addr);
        rtnl_neigh_set_dst (rn, addr);
        nl_addr_put (addr);
    }
    if (lladdr)
    {
        struct nl_addr *addr;
        nl_addr_parse (lladdr, AF_LLC, &addr);
        NP_ASSERT_NOT_NULL (addr);
        rtnl_neigh_set_lladdr (rn, addr);
        nl_addr_put (addr);
    }
    rtnl_neigh_set_state (rn, NUD_REACHABLE);
    return (struct nl_object *) rn;
}

static void
setup_test (char *ignore)
{
    np_mock (if_indextoname, mock_if_indextoname);
    np_mock (apteryx_set_tree_full, mock_apteryx_set_tree_full);
    np_mock (apteryx_set_full, mock_apteryx_set_full);
    np_mock (apteryx_prune, mock_apteryx_prune_path);
    apteryx_tree = NULL;
    apteryx_path = NULL;
    apteryx_value = NULL;
    apteryx_prune_path = NULL;
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_neighbor_invalid ()
{
    setup_test ("invalid neighbor cb action");
    struct nl_object *neigh = make_neighbor (0, NULL, NULL);
    nl_neighbor_cb (-1, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_neighbor_null ()
{
    setup_test ("missing neighbor object");
    nl_neighbor_cb (NL_ACT_NEW, NULL, NULL);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_neighbor_incomplete ()
{
    setup_test ("invalid neighbor family");
    struct nl_object *neigh = make_neighbor (0, NULL, NULL);
    nl_neighbor_cb (NL_ACT_NEW, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_neighbor_ipv4 ()
{
    setup_test (NULL);
    struct nl_object *neigh = make_neighbor (AF_INET, DST_ADDRESS, LL_ADDRESS);
    nl_neighbor_cb (NL_ACT_NEW, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv4_PATH,
            INTERFACES_STATE_IPV4_NEIGHBOR_IP, DST_ADDRESS));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv4_PATH,
            INTERFACES_STATE_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, LL_ADDRESS));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv4_PATH,
            INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN,
            INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN_DYNAMIC));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_neighbor_ipv6 ()
{
    setup_test (NULL);
    struct nl_object *neigh = make_neighbor (AF_INET6, DST6_ADDRESS, LL_ADDRESS);
    nl_neighbor_cb (NL_ACT_NEW, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv6_PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_IP, DST6_ADDRESS));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv6_PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, LL_ADDRESS));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv6_PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN,
            INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN_DYNAMIC));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv6_PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_IS_ROUTER,
            NULL));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPv6_PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_STATE,
            INTERFACES_STATE_IPV6_NEIGHBOR_STATE_REACHABLE));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}
