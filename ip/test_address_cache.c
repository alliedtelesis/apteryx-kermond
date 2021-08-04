/**
 * @file test_address_cache.c
 * Unit tests for IP address cache
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
#include "address-cache.c"

#include "test.h"

#define IFPATH      INTERFACES_STATE_PATH"/"IFNAME
#define IPV4PATH    IFPATH"/"INTERFACES_STATE_IPV4_ADDRESS"/"ADDRV4
#define IPV6PATH    IFPATH"/"INTERFACES_STATE_IPV6_ADDRESS"/"ADDRV6

static GNode*
apteryx_get_node (GNode *tree, char *path)
{
    if (!tree || !path)
        return NULL;
    char *npath = apteryx_node_path (tree);
    if (!npath || strcmp (npath, path) != 0)
    {
        GNode *node = g_node_first_child (tree);
        tree = NULL;
        while (node && !tree)
        {
            tree = apteryx_get_node (node, path);
            node = g_node_next_sibling (node);
        }
    }
    free (npath);
    return tree;
}

static bool
check_tree_parameter (GNode *tree, char *path, char *value)
{
    GNode *node = apteryx_get_node (tree, path);
    if (!value && node)
        return false;
    if (value && !node)
        return false;
    if (value && node && strcmp (APTERYX_VALUE (node), value) != 0)
        return false;
    return true;
}

static struct nl_object *
make_address (int family, char *ip, int prefixlen)
{
    struct rtnl_addr *ra = rtnl_addr_alloc ();
    rtnl_addr_set_family (ra, family);
    if (ip)
    {
        struct nl_addr *addr;
        nl_addr_parse (ip, family, &addr);
        NP_ASSERT_NOT_NULL (addr);
        nl_addr_set_prefixlen (addr, prefixlen);
        rtnl_addr_set_local (ra, addr);
        nl_addr_put (addr);
    }
    return (struct nl_object *) ra;
}

static void
setup_test (char *ignore)
{
    np_mock (if_indextoname, mock_if_indextoname);
    np_mock (apteryx_set_tree_full, mock_apteryx_set_tree_full);
    np_mock (apteryx_set_full, mock_apteryx_set_full);
    np_mock (apteryx_prune, mock_apteryx_prune_path);
    link_active = true;
    apteryx_tree = NULL;
    apteryx_path = NULL;
    apteryx_value = NULL;
    apteryx_prune_path = NULL;
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_address_invalid ()
{
    NP_TEST_START
    setup_test ("invalid address cb action");
    struct nl_object *addr = make_address (0, NULL, 0);
    nl_address_cb (-1, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("ADDRESS-CACHE: invalid address cb action:-1\n")
}

void test_address_null ()
{
    NP_TEST_START
    setup_test ("missing address object");
    nl_address_cb (NL_ACT_NEW, NULL, NULL);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("ADDRESS-CACHE: missing address object\n")
}

void test_address_incomplete ()
{
    NP_TEST_START
    setup_test ("invalid address family");
    struct nl_object *addr = make_address (0, NULL, 0);
    nl_address_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("ADDRESS-CACHE: invalid address family:0\n")
}

void test_address_ipv4 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *addr = make_address (AF_INET, ADDRV4, 24);
    nl_address_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH"/"
            INTERFACES_STATE_IPV4_ADDRESS_IP, ADDRV4));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH"/"
            INTERFACES_STATE_IPV4_ADDRESS_SUBNET_PREFIX_LENGTH, "24"));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH"/"
            INTERFACES_STATE_IPV4_ADDRESS_ORIGIN,
            INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN_OTHER));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_address_ipv6 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *addr = make_address (AF_INET6, ADDRV6, 64);
    nl_address_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH"/"
            INTERFACES_STATE_IPV6_ADDRESS_IP, ADDRV6));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH"/"
            INTERFACES_STATE_IPV6_ADDRESS_PREFIX_LENGTH, "64"));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH"/"
            INTERFACES_STATE_IPV6_ADDRESS_ORIGIN,
            INTERFACES_STATE_IPV6_ADDRESS_ORIGIN_OTHER));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH"/"
            INTERFACES_STATE_IPV6_ADDRESS_STATUS,
            INTERFACES_STATE_IPV6_ADDRESS_STATUS_UNKNOWN));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}
