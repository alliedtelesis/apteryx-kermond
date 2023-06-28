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

#include "test.h"

#define PATHV4      INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_ADDRESS"/"ADDRV4"/"
#define PATHV6      INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_ADDRESS"/"ADDRV6"/"

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
    NP_TEST_START
    setup_test (true, "Invalid static address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (NULL, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Invalid static address: (null) = 1\n")
}

void test_static_addr4_path_invalid ()
{
    NP_TEST_START
    setup_test (true, "Invalid static address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4 "dog", "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("")
}

void test_static_addr4_ip_invalid ()
{
    NP_TEST_START
    setup_test (true, "Unable to parse ip");
    NP_ASSERT_TRUE (apteryx_static_address_cb (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV4_ADDRESS"/999.1_/"
            INTERFACES_STATE_IPV4_ADDRESS_IP, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Unable to parse ip: Invalid address for specified address family\n")
}

void test_static_addr4_prefixlen_invalid ()
{
    NP_TEST_START
    setup_test (true, "Unable to parse phys-address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_SUBNET_PREFIX_LENGTH, "9999999"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("")
}

void test_static_addr4_add_interface_inactive ()
{
    NP_TEST_START
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, LLADDR));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Link \"eth99\" is not currently active\n")
}

void test_static_addr4_add()
{
    NP_TEST_START
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, LLADDR));
    NP_ASSERT_NOT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    assert_address_valid (address_added, AF_INET);
    NP_TEST_END ("")
}

void test_static_addr4_add_interface_go_active ()
{
    NP_TEST_START
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
    NP_TEST_END ("")
}

void test_static_addr4_delete_interface_inactive ()
{
    NP_TEST_START
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Link \"eth99\" is not currently active\n")
}

void test_static_addr4_delete ()
{
    NP_TEST_START
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV4
            INTERFACES_STATE_IPV4_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NOT_NULL (address_deleted);
    assert_address_valid (address_deleted, AF_INET);
    NP_TEST_END ("")
}

void test_static_addr6_path_null ()
{
    NP_TEST_START
    setup_test (true, "Invalid static address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (NULL, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Invalid static address: (null) = 1\n")
}

void test_static_addr6_path_invalid ()
{
    NP_TEST_START
    setup_test (true, "Unexpected static address parameter");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6 "dog", "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("")
}

void test_static_addr6_ip_invalid ()
{
    NP_TEST_START
    setup_test (true, "Unable to parse ip");
    NP_ASSERT_TRUE (apteryx_static_address_cb (
            INTERFACES_PATH"/"IFNAME"/"INTERFACES_IPV6_ADDRESS"/999.1_/"
            INTERFACES_IPV6_ADDRESS_IP, "1"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Unable to parse ip: Invalid address for specified address family\n")
}

void test_static_addr6_prefixlen_invalid ()
{
    NP_TEST_START
    setup_test (true, "Unable to parse phys-address");
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_PREFIX_LENGTH, "9999999"));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("")
}

void test_static_addr6_add_interface_inactive ()
{
    NP_TEST_START
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, ADDRV6));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Link \"eth99\" is not currently active\n")
}

void test_static_addr6_add ()
{
    NP_TEST_START
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, ADDRV6));
    NP_ASSERT_NOT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    assert_address_valid (address_added, AF_INET6);
    NP_TEST_END ("")
}

void test_static_addr6_add_interface_go_active ()
{
    NP_TEST_START
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
    NP_TEST_END ("")
}

void test_static_addr6_delete_interface_inactive ()
{
    NP_TEST_START
    setup_test (false, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NULL (address_deleted);
    NP_TEST_END ("ADDRESS: Link \"eth99\" is not currently active\n")
}

void test_static_addr6_delete ()
{
    NP_TEST_START
    setup_test (true, NULL);
    NP_ASSERT_TRUE (apteryx_static_address_cb (PATHV6
            INTERFACES_IPV6_ADDRESS_IP, NULL));
    NP_ASSERT_NULL (address_added);
    NP_ASSERT_NOT_NULL (address_deleted);
    assert_address_valid (address_deleted, AF_INET6);
    NP_TEST_END ("")
}
