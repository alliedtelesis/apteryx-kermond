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

#include <np.h>

static GList *cmds = NULL;

#define PATH    NEIGHBOR_IPV4_SETTINGS_INTERFACES_PATH "/eth1/"

static int
mock_system (const char *cmd)
{
    GList *found = g_list_find_custom (cmds, cmd, (GCompareFunc) strcmp);
    NP_ASSERT_NOT_NULL (found);
    if (found)
    {
        free (found->data);
        cmds = g_list_delete_link (cmds, found);
    }
    return 0;
}

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
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv4_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_invalid_path ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv4_settings (NEIGHBOR_IPV4_PATH, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_invalid_parameter ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv4_settings (NEIGHBOR_IPV4_SETTINGS_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
}

/* Opportunistic Neighbor Discovery*******************************************/

void test_neigh_opp_nd_value_null ()
{
    setup_test ("sysctl -w net.ipv4.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_PATH, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_opp_nd_value_disable ()
{
    setup_test ("sysctl -w net.ipv4.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_PATH, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_opp_nd_value_enable ()
{
    setup_test ("sysctl -w net.ipv4.aggressive_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_PATH, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_opp_nd_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.aggressive_nd=0", "Invalid opportunistic-nd");
    NP_ASSERT_TRUE (watch_ipv4_settings (NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
}

/* Aging Timeout *************************************************************/

void test_neigh_aging_timeout_value_null ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_aging_timeout_value_0 ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_aging_timeout_value_432000 ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=432000000", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT, "432000"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_aging_timeout_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", "Invalid aging-timeout");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT, "dog"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_aging_timeout_value_too_low ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", "Invalid aging-timeout");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT, "-1"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_aging_timeout_value_too_high ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.base_reachable_time_ms=300000", "Invalid aging-timeout");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT, "432001"));
    NP_ASSERT_NULL (cmds);
}

/* MAC Disparity *************************************************************/

void test_neigh_mac_disparity_value_null ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_mac_disparity_value_disable ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_mac_disparity_value_enable ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_mac_disparity_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.arp_mac_disparity=0", "Invalid mac-disparity");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY, "dog"));
    NP_ASSERT_NULL (cmds);
}

/* Proxy ARP *****************************************************************/

void test_neigh_proxy_arp_value_null ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_proxy_arp_value_disable ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_proxy_arp_value_enable ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_proxy_arp_value_local ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=2", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP, "2"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_proxy_arp_value_both ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=3", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP, "3"));
    NP_ASSERT_NULL (cmds);
}


void test_neigh_proxy_arp_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.conf.eth1.proxy_arp=0", "Invalid proxy-arp");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP, "dog"));
    NP_ASSERT_NULL (cmds);
}

/* MAC Disparity *************************************************************/

void test_neigh_interface_opp_nd_value_null ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_interface_opp_nd_value_disable ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_interface_opp_nd_value_enable ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_neigh_interface_opp_nd_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.neigh.eth1.optimistic_nd=1", "Invalid opportunistic-nd");
    NP_ASSERT_TRUE (watch_ipv4_settings (PATH
            NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND, "dog"));
    NP_ASSERT_NULL (cmds);
}

/* IPv6 Settings *************************************************************/

void test_neighv6_path_null ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv6_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_neighv6_invalid_path ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv6_settings (NEIGHBOR_IPV6_PATH, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_neighv6_invalid_parameter ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_ipv6_settings (NEIGHBOR_IPV6_SETTINGS_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
}

/* Opportunistic Neighbor Discovery (v6) *************************************/

void test_neighv6_opp_nd_value_null ()
{
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv6_settings (NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_PATH, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_neighv6_opp_nd_value_disable ()
{
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=0", NULL);
    NP_ASSERT_TRUE (watch_ipv6_settings (NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_PATH, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_neighv6_opp_nd_value_enable ()
{
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=1", NULL);
    NP_ASSERT_TRUE (watch_ipv6_settings (NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_PATH, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_neighv6_opp_nd_value_invalid ()
{
    setup_test ("sysctl -w net.ipv6.icmp.aggressive_nd=0", "Invalid opportunistic-nd");
    NP_ASSERT_TRUE (watch_ipv6_settings (NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
}
