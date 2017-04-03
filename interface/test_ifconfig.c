/**
 * @file test_ifconfig.c
 * Unit tests for Interface configuration
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
#include "ifconfig.c"

#include <np.h>

#define PATH    INTERFACE_INTERFACES_PATH "/eth1/" INTERFACE_INTERFACES_SETTINGS "/"

static bool link_active;
static struct rtnl_link *
mock_rtnl_link_get_by_name (struct nl_cache *cache, const char *name)
{
    return link_active ? rtnl_link_alloc () : NULL;
}

static struct rtnl_link *link_changes;
static int
mock_rtnl_link_change (struct nl_sock *sk, struct rtnl_link *orig,
             struct rtnl_link *changes, int flags)
{
    NP_ASSERT_NULL (link_changes);
    link_changes = (struct rtnl_link *) nl_object_clone ((struct nl_object *) changes);
    return 0;
}

static GNode *apteryx_tree;
static GNode *
mock_apteryx_get_tree (const char *path)
{
    GNode *tree = apteryx_tree;
    apteryx_tree = NULL;
    return tree;
}

static void
setup_test (bool active, char *parameter, char *value, char *ignore)
{
    link_active = active;
    if (parameter && value)
    {
        GNode *node;
        apteryx_tree = g_node_new (strdup (INTERFACE_INTERFACES_PATH));
        node = APTERYX_NODE(apteryx_tree, strdup ("eth1"));
        node = APTERYX_NODE(node, strdup (INTERFACE_INTERFACES_SETTINGS));
        APTERYX_LEAF (node, strdup (parameter), strdup (value));
    }
    link_cache = (struct nl_cache *) ~0;
    np_mock (rtnl_link_change, mock_rtnl_link_change);
    np_mock (rtnl_link_get_by_name, mock_rtnl_link_get_by_name);
    np_mock (apteryx_get_tree, mock_apteryx_get_tree);
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_ifconfig_path_null ()
{
    setup_test (true, NULL, NULL, "Invalid interface settings path");
    NP_ASSERT_FALSE (apteryx_if_cb (NULL, "1"));
    NP_ASSERT_NULL (link_changes);
}

/**
 * admin-status
 */
void test_ifconfig_admin_value_null ()
{
    setup_test (true, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "admin-status", NULL));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_flags (link_changes), IFF_UP);
}

void test_ifconfig_admin_value_invalid ()
{
    setup_test (true, NULL, NULL, "Invalid admin-status");
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "admin-status", "dog"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_flags (link_changes), IFF_UP);
}

void test_ifconfig_admin_down ()
{
    setup_test (true, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "admin-status", "0"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_flags (link_changes), 0);
}

void test_ifconfig_admin_up ()
{
    setup_test (true, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "admin-status", "1"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_flags (link_changes), IFF_UP);
}

void test_ifconfig_admin_inactive_down ()
{
    setup_test (false, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "admin-status", "0"));
    NP_ASSERT_NULL (link_changes);
}

void test_ifconfig_admin_inactive_up ()
{
    setup_test (false, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "admin-status", "1"));
    NP_ASSERT_NULL (link_changes);
}

void test_ifconfig_admin_to_active_default ()
{
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, "eth0");
    setup_test (true, NULL, NULL, NULL);
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NULL (link_changes);
}

void test_ifconfig_admin_to_active_down ()
{
    setup_test (true, "admin-status", "0", NULL);
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, "eth0");
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_flags (link_changes), 0);
}

/**
 * mtu
 */
void test_ifconfig_mtu_value_null ()
{
    setup_test (true, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", NULL));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 1500);
}

void test_ifconfig_mtu_value_invalid ()
{
    setup_test (true, NULL, NULL, "Invalid MTU");
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "dog"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 1500);
}

void test_ifconfig_mtu_too_low ()
{
    setup_test (true, NULL, NULL, "Invalid MTU");
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "67"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 1500);
}

void test_ifconfig_mtu_too_high ()
{
    setup_test (true, NULL, NULL, "Invalid MTU");
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "16536"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 1500);
}

void test_ifconfig_mtu_68 ()
{
    setup_test (true, NULL, NULL, "Invalid MTU");
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "68"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 68);
}

void test_ifconfig_mtu_16535 ()
{
    setup_test (true, NULL, NULL, "Invalid MTU");
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "16535"));
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 16535);
}

void test_ifconfig_mtu_inactive_default ()
{
    setup_test (false, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "1500"));
    NP_ASSERT_NULL (link_changes);
}

void test_ifconfig_mtu_inactive_1400 ()
{
    setup_test (false, NULL, NULL, NULL);
    NP_ASSERT_TRUE (apteryx_if_cb (PATH "mtu", "1400"));
    NP_ASSERT_NULL (link_changes);
}

void test_ifconfig_mtu_to_active_default ()
{
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, "eth0");
    setup_test (true, NULL, NULL, NULL);
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NULL (link_changes);
}

void test_ifconfig_mtu_to_active_1400 ()
{
    setup_test (true, "mtu", "1400", NULL);
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_name (link, "eth0");
    nl_if_cb (NL_ACT_NEW, NULL, (struct nl_object *) link);
    rtnl_link_put (link);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NOT_NULL (link_changes);
    NP_ASSERT_EQUAL (rtnl_link_get_mtu (link_changes), 1400);
}
