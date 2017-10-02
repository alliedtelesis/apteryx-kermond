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

#include <np.h>

#define IFNAME      "eth1"
#define IFINDEX     "99"
#define IFPATH      INTERFACES_STATE_PATH"/"IFNAME
#define ADDRV4      "192.168.1.1"
#define ADDRV6      "fc00::2"
#define IPV4PATH    IFPATH"/"INTERFACES_STATE_IPV4_ADDRESS"/"ADDRV4
#define IPV6PATH    IFPATH"/"INTERFACES_STATE_IPV6_ADDRESS"/"ADDRV6

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
make_address (int family, char *ip, int prefixlen)
{
    struct rtnl_addr *ra = rtnl_addr_alloc ();
    rtnl_addr_set_family (ra, family);
    if (ip)
    {
        struct nl_addr *addr;
        nl_addr_parse (ip, family, &addr);
        NP_ASSERT_NOT_NULL (addr);
        rtnl_addr_set_local (ra, addr);
        nl_addr_put (addr);
    }
    rtnl_addr_set_prefixlen (ra, prefixlen);
    return (struct nl_object *) ra;
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

void test_address_invalid ()
{
    setup_test ("invalid address cb action");
    struct nl_object *addr = make_address (0, NULL, 0);
    nl_address_cb (-1, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_address_null ()
{
    setup_test ("missing address object");
    nl_address_cb (NL_ACT_NEW, NULL, NULL);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_address_incomplete ()
{
    setup_test ("invalid address family");
    struct nl_object *addr = make_address (0, NULL, 0);
    nl_address_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_address_ipv4 ()
{
    setup_test (NULL);
    struct nl_object *addr = make_address (AF_INET, ADDRV4, 24);
    nl_address_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH,
            INTERFACES_STATE_IPV4_ADDRESS_IP, ADDRV4));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH,
            INTERFACES_STATE_IPV4_ADDRESS_PREFIX_LENGTH, "24"));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV4PATH,
            INTERFACES_STATE_IPV4_ADDRESS_ORIGIN,
            INTERFACES_STATE_IPV4_ADDRESS_ORIGIN_STATIC));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}

void test_address_ipv6 ()
{
    setup_test (NULL);
    struct nl_object *addr = make_address (AF_INET6, ADDRV6, 64);
    nl_address_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_ADDRESS_IP, ADDRV6));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_ADDRESS_PREFIX_LENGTH, "64"));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_ADDRESS_ORIGIN,
            INTERFACES_STATE_IPV6_ADDRESS_ORIGIN_STATIC));
    NP_ASSERT (check_tree_parameter (apteryx_tree, IPV6PATH,
            INTERFACES_STATE_IPV6_ADDRESS_STATUS,
            INTERFACES_STATE_IPV6_ADDRESS_STATUS_PREFERRED));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
}
