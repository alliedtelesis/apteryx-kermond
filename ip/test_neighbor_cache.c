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

#include "test.h"

#define IFPATH      INTERFACES_STATE_PATH"/"IFNAME
#define IPV4PATH    IFPATH"/"INTERFACES_STATE_IPV4_NEIGHBOR"/"ADDRV4
#define IPV6PATH    IFPATH"/"INTERFACES_STATE_IPV6_NEIGHBOR"/"ADDRV6

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
    rtnl_neigh_set_ifindex (rn, IFINDEX);
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
    NP_TEST_START
    setup_test ("invalid neighbor cb action");
    struct nl_object *neigh = make_neighbor (0, NULL, NULL);
    nl_neighbor_cb (-1, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("NEIGHBOR-CACHE: invalid neighbor cb action:-1\n")
}

void test_neighbor_null ()
{
    NP_TEST_START
    setup_test ("missing neighbor object");
    nl_neighbor_cb (NL_ACT_NEW, NULL, NULL);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("NEIGHBOR-CACHE: missing neighbor object\n")
}

void test_neighbor_incomplete ()
{
    NP_TEST_START
    setup_test ("invalid neighbor family");
    struct nl_object *neigh = make_neighbor (0, NULL, NULL);
    nl_neighbor_cb (NL_ACT_NEW, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("NEIGHBOR-CACHE: invalid neighbor family:0\n")
}

void test_neighbor_ipv4 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *neigh = make_neighbor (AF_INET, ADDRV4, LLADDR);
    nl_neighbor_cb (NL_ACT_NEW, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH,
            INTERFACES_STATE_IPV4_NEIGHBOR_IP, ADDRV4));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH,
            INTERFACES_STATE_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS, LLADDR));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH,
            INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN,
            INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN_DYNAMIC));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_neighbor_ipv6 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *neigh = make_neighbor (AF_INET6, ADDRV6, LLADDR);
    nl_neighbor_cb (NL_ACT_NEW, NULL, neigh);
    nl_object_put (neigh);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_IP, ADDRV6));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS, LLADDR));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN,
            INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN_DYNAMIC));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_IS_ROUTER,
            NULL));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_NEIGHBOR_STATE,
            INTERFACES_STATE_IPV6_NEIGHBOR_STATE_REACHABLE));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}
