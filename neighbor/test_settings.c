/**
 * @file test_settings.c
 * Unit tests for neighbor configuration
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
#include "settings.c"

#include "test.h"

#define PATH    IP_NEIGHBOR_IPV4_INTERFACES_PATH "/eth1/"

static void
setup_test (char *cmd, char *ignore)
{
    if (cmd)
        cmds = g_list_append (cmds, strdup (cmd));
    np_mock (system, mock_system);
    if (ignore)
        np_syslog_ignore (ignore);
}

/* IPv4 Settings *************************************************************/

void test_neigh_path_null ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv4_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Unexpected path: (null)\n");
}

void test_neigh_invalid_path ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv4_settings (IP_NEIGHBOR_IPV4_PATH, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Unexpected path: /ip/neighbor/ipv4\n");
}

void test_neigh_invalid_parameter ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv4_settings (IP_NEIGHBOR_IPV4_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Unexpected path: /ip/neighbor/ipv4/speed\n");
}

/* Opportunistic Neighbor Discovery*******************************************/

void test_neigh_opp_nd_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (IP_NEIGHBOR_IPV4_OPPORTUNISTIC_ND_PATH, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_opp_nd_value_disable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (IP_NEIGHBOR_IPV4_OPPORTUNISTIC_ND_PATH, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_opp_nd_value_enable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.aggressive_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (IP_NEIGHBOR_IPV4_OPPORTUNISTIC_ND_PATH, "1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_opp_nd_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.aggressive_nd=0", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (IP_NEIGHBOR_IPV4_OPPORTUNISTIC_ND_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/neighbor/ipv4/opportunistic-nd value (dog) using default (0)\n");
}

/* Aging Timeout *************************************************************/

void test_neigh_aging_timeout_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_aging_timeout_value_0 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_aging_timeout_value_432000 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=432000000", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT, "432000"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_aging_timeout_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Invalid aging-timeout value (dog) using default (300)\n");
}

void test_neigh_aging_timeout_value_too_low ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT, "-1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Invalid aging-timeout value (-1) using default (300)\n");
}

void test_neigh_aging_timeout_value_too_high ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT, "432001"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Invalid aging-timeout value (432001) using default (300)\n");
}

/* MAC Disparity *************************************************************/

void test_neigh_mac_disparity_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_MAC_DISPARITY, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_mac_disparity_value_disable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_MAC_DISPARITY, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_mac_disparity_value_enable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_MAC_DISPARITY, "1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_mac_disparity_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=0", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_MAC_DISPARITY, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/neighbor/ipv4/interfaces/eth1/mac-disparity value (dog) using default (0)\n");
}

/* Proxy ARP *****************************************************************/

void test_neigh_proxy_arp_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_proxy_arp_value_disable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP, "disabled"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_proxy_arp_value_enable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP, "enabled"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_proxy_arp_value_local ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=2", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP, "local"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_proxy_arp_value_both ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=3", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP, "both"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}


void test_neigh_proxy_arp_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=0", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/neighbor/ipv4/interfaces/eth1/proxy-arp value (dog) using default (0)\n");
}

/* MAC Disparity *************************************************************/

void test_neigh_interface_opp_nd_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_OPTIMISTIC_ND, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_interface_opp_nd_value_disable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_OPTIMISTIC_ND, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_interface_opp_nd_value_enable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_OPTIMISTIC_ND, "1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neigh_interface_opp_nd_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=1", "Invalid");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            IP_NEIGHBOR_IPV4_INTERFACES_OPTIMISTIC_ND, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/neighbor/ipv4/interfaces/eth1/optimistic-nd value (dog) using default (1)\n");
}

/* IPv6 Settings *************************************************************/

void test_neighv6_path_null ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv6_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Unexpected path: (null)\n");
}

void test_neighv6_invalid_path ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv6_settings (IP_NEIGHBOR_IPV6_PATH, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Unexpected path: /ip/neighbor/ipv6\n");
}

void test_neighv6_invalid_parameter ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv6_settings (IP_NEIGHBOR_IPV6_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("NEIGHBOR: Unexpected path: /ip/neighbor/ipv6/speed\n");
}

/* Opportunistic Neighbor Discovery (v6) *************************************/

void test_neighv6_opp_nd_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv6_settings (IP_NEIGHBOR_IPV6_OPPORTUNISTIC_ND_PATH, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neighv6_opp_nd_value_disable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv6_settings (IP_NEIGHBOR_IPV6_OPPORTUNISTIC_ND_PATH, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neighv6_opp_nd_value_enable ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv6_settings (IP_NEIGHBOR_IPV6_OPPORTUNISTIC_ND_PATH, "1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_neighv6_opp_nd_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=0", "Invalid");
    NP_ASSERT_TRUE (watch_ipv6_settings (IP_NEIGHBOR_IPV6_OPPORTUNISTIC_ND_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/neighbor/ipv6/opportunistic-nd value (dog) using default (0)\n");
}
