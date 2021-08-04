/**
 * @file test_ifstatus.c
 * Unit tests for Interface status
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
#include "ifstatus.c"
#include <netinet/ether.h>

#include "test.h"

static void
assert_tree_parameter (GNode *tree, char *iface, char *mode, char *parameter, char *value)
{
    GNode *node;
    NP_ASSERT_NOT_NULL (apteryx_find_child (apteryx_tree, "interface"));
    node = apteryx_find_child (apteryx_tree, "interface");
    NP_ASSERT_NOT_NULL (apteryx_find_child (node, "interfaces"));
    node = apteryx_find_child (node, "interfaces");
    NP_ASSERT_NOT_NULL (apteryx_find_child (node, iface));
    node = apteryx_find_child (node, iface);
    if (mode)
    {
        NP_ASSERT_NOT_NULL (apteryx_find_child (node, mode));
        node = apteryx_find_child (node, mode);
    }
    NP_ASSERT_NOT_NULL (apteryx_find_child (node, parameter));
    node = apteryx_find_child (node, parameter);
    NP_ASSERT_NOT_NULL (APTERYX_VALUE (node));
    NP_ASSERT_STR_EQUAL (APTERYX_VALUE (node), value);
}

static struct nl_object *
make_link (char *iface)
{
    struct rtnl_link *link = rtnl_link_alloc ();
    rtnl_link_set_ifindex (link, IFINDEX);
    if (iface)
        rtnl_link_set_name (link, iface);
    return (struct nl_object *) link;
}

static void
setup_test (char *ignore)
{
    np_mock (apteryx_set_tree_full, mock_apteryx_set_tree_full);
    np_mock (apteryx_set_full, mock_apteryx_set_full);
    np_mock (apteryx_prune, mock_apteryx_prune_path);
    np_mock (procfs_read_uint32, mock_procfs_read_uint32);
    np_mock (procfs_read_string, mock_procfs_read_string);
    apteryx_tree = NULL;
    apteryx_path = NULL;
    apteryx_value = NULL;
    apteryx_prune_path = NULL;
    procfs_uint32_t = 0;
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_ifstatus_action_invalid ()
{
    NP_TEST_START
    setup_test ("invalid interface cb action");
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (-1, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("IFSTATUS: invalid interface cb action:-1\n")
}

void test_ifstatus_link_null ()
{
    NP_TEST_START
    setup_test ("missing link object");
    nl_if_cb (NL_ACT_NEW, NULL, NULL);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("IFSTATUS: missing link object\n")
}

void test_ifstatus_link_incomplete ()
{
    NP_TEST_START
    setup_test ("invalid link object");
    struct nl_object *link = make_link (NULL);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("IFSTATUS: invalid link object\n")
}

void test_ifstatus_link_new ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME, NULL,
            INTERFACE_INTERFACES_NAME, IFNAME);
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_link_alias ()
{
    NP_TEST_START
    GNode *node;
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    NP_ASSERT_NOT_NULL (apteryx_find_child (apteryx_tree, "interface"));
    node = apteryx_find_child (apteryx_tree, "interface");
    NP_ASSERT_NOT_NULL (apteryx_find_child (node, "if-alias"));
    node = apteryx_find_child (node, "if-alias");
    NP_ASSERT_NOT_NULL (apteryx_find_child (node, SIFINDEX));
    node = apteryx_find_child (node, SIFINDEX);
    NP_ASSERT_NOT_NULL (APTERYX_VALUE (node));
    NP_ASSERT_STR_EQUAL (APTERYX_VALUE (node), IFNAME);
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_link_del ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_DEL, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NULL (apteryx_tree);
    NP_ASSERT_NOT_NULL (apteryx_path);
    NP_ASSERT_STR_EQUAL (apteryx_path, INTERFACE_IF_ALIAS_PATH"/"SIFINDEX);
    free (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NOT_NULL (apteryx_prune_path);
    NP_ASSERT_STR_EQUAL (apteryx_prune_path,
            INTERFACE_INTERFACES_PATH"/"IFNAME"/"INTERFACE_INTERFACES_STATUS);
    free (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_admin_status_default_down ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "admin-status",
            xstr (INTERFACE_INTERFACES_STATUS_ADMIN_STATUS_ADMIN_DOWN));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_admin_status_up ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_flags ((struct rtnl_link *) link, IFF_UP);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "admin-status",
            xstr (INTERFACE_INTERFACES_STATUS_ADMIN_STATUS_ADMIN_UP));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_oper_status ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_operstate ((struct rtnl_link *) link, IF_OPER_UP);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "oper-status",
            "6");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_flags ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_flags ((struct rtnl_link *) link, IFF_UP|IFF_RUNNING);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "flags",
            "65");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_phys_address ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    struct nl_addr *addr;
    addr = nl_addr_build (AF_LLC, ether_aton ("00:11:22:33:44:55"), ETH_ALEN);
    rtnl_link_set_addr((struct rtnl_link *) link, addr);
    nl_addr_put (addr);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "phys-address",
            "00:11:22:33:44:55");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_promisc_default_off ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "promisc",
            xstr (INTERFACE_INTERFACES_STATUS_PROMISC_PROMISC_OFF));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_promisc_on ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_promiscuity ((struct rtnl_link *) link, 1);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "promisc",
            xstr (INTERFACE_INTERFACES_STATUS_PROMISC_PROMISC_ON));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_qdisc ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_qdisc ((struct rtnl_link *) link, "noqueue");
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "qdisc",
            "noqueue");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_mtu_default_1500 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "mtu",
            xstr (INTERFACE_INTERFACES_STATUS_MTU_DEFAULT));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_mtu_68 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_mtu ((struct rtnl_link *) link, 68);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "mtu",
            "68");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_mtu_16535 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_mtu ((struct rtnl_link *) link, 16535);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "mtu",
            "16535");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_speed_default ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "speed",
            "0");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_speed_1000 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    procfs_uint32_t = 1000;
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "speed",
            "1000");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_duplex_default_auto ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "duplex",
            xstr (INTERFACE_INTERFACES_STATUS_DUPLEX_AUTO));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_duplex_full ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    procfs_string = "full";
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "duplex",
            xstr (INTERFACE_INTERFACES_STATUS_DUPLEX_FULL));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_duplex_half ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    procfs_string = "half";
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "duplex",
            xstr (INTERFACE_INTERFACES_STATUS_DUPLEX_HALF));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_arptype_default_void ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "arptype",
            xstr (INTERFACE_INTERFACES_STATUS_ARPTYPE_VOID));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_arptype_ethernet ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_arptype ((struct rtnl_link *) link, ARPHRD_ETHER);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "arptype",
            xstr (INTERFACE_INTERFACES_STATUS_ARPTYPE_ETHERNET));
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_rxq_default_1 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "rxq",
            "1");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_rxq_2 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_num_rx_queues ((struct rtnl_link *) link, 2);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "rxq",
            "2");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_txqlen_default_1000 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "txqlen",
            "1000");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_txqlen_2000 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_txqlen ((struct rtnl_link *) link, 2000);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "txqlen",
            "2000");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_txq_default_1 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "txq",
            "1");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}

void test_ifstatus_txq_2 ()
{
    NP_TEST_START
    setup_test (NULL);
    struct nl_object *link = make_link (IFNAME);
    rtnl_link_set_num_tx_queues ((struct rtnl_link *) link, 2);
    nl_if_cb (NL_ACT_NEW, NULL, link);
    nl_object_put (link);
    NP_ASSERT_NOT_NULL (apteryx_tree);
    assert_tree_parameter (apteryx_tree, IFNAME,
            INTERFACE_INTERFACES_STATUS, "txq",
            "2");
    apteryx_free_tree (apteryx_tree);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_ASSERT_NULL (apteryx_prune_path);
    NP_TEST_END ("")
}
