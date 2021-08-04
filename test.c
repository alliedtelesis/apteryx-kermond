/**
 * @file test.c
 * Globals required for test build
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
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include "kermond.h"
#include "test.h"

#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/neighbour.h>

/* Mainloop handle */
GMainLoop *g_loop = NULL;

/* Debug */
bool kermond_debug = true;
bool kermond_verbose = false;

GList *cmds = NULL;

int
__wrap_system (const char *cmd)
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

bool link_active = false;
int addr_family = AF_INET;

unsigned int
__wrap_if_nametoindex (const char *ifname)
{
    return link_active ? IFINDEX : 0;
}

char *
__wrap_if_indextoname (unsigned int ifindex, char *ifname)
{
    if (link_active)
    {
        strcpy (ifname, IFNAME);
        return ifname;
    }
    else
    {
        return NULL;
    }
}

struct rtnl_addr *address_added = NULL;
int
__wrap_rtnl_addr_add(struct nl_sock *sk, struct rtnl_addr *tmpl, int flags)
{
    NP_ASSERT_NULL (address_added);
    address_added = (struct rtnl_addr *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

struct rtnl_addr *address_deleted = NULL;
int
__wrap_rtnl_addr_delete(struct nl_sock *sk, struct rtnl_addr *tmpl, int flags)
{
    NP_ASSERT_NULL (address_deleted);
    address_deleted = (struct rtnl_addr *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

struct rtnl_link *
__wrap_rtnl_link_get_by_name (struct nl_cache *cache, const char *name)
{
    return link_active ? rtnl_link_alloc () : NULL;
}

struct rtnl_link *link_changes = NULL;
int
__wrap_rtnl_link_change (struct nl_sock *sk, struct rtnl_link *orig,
             struct rtnl_link *changes, int flags)
{
    g_assert_null (link_changes);
    link_changes = (struct rtnl_link *) nl_object_clone ((struct nl_object *) changes);
    return 0;
}

void
__wrap_nl_cache_foreach_filter (struct nl_cache *cache, struct nl_object *filter,
        void (*cb)(struct nl_object *, void *), void *arg)
{
    struct nl_addr *a;
    if (addr_family == AF_INET)
        nl_addr_parse (IP4ADDR, addr_family, &a);
    else if (addr_family == AF_INET6)
        nl_addr_parse (IP6ADDR, addr_family, &a);
    else
        return;
    struct rtnl_addr *addr = rtnl_addr_alloc ();
    rtnl_addr_set_family (addr, addr_family);
    rtnl_addr_set_ifindex (addr, IFINDEX);
    rtnl_addr_set_local (addr, a);
    cb ((struct nl_object *) addr, arg);
    rtnl_addr_put (addr);
    nl_addr_put (a);
}

struct rtnl_neigh *neighbor_added = NULL;
int
__wrap_rtnl_neigh_add(struct nl_sock *sk, struct rtnl_neigh *tmpl, int flags)
{
    NP_ASSERT_NULL (neighbor_added);
    neighbor_added = (struct rtnl_neigh *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

struct rtnl_neigh *neighbor_deleted = NULL;
int
__wrap_rtnl_neigh_delete(struct nl_sock *sk, struct rtnl_neigh *tmpl, int flags)
{
    NP_ASSERT_NULL (neighbor_deleted);
    neighbor_deleted = (struct rtnl_neigh *) nl_object_clone ((struct nl_object *) tmpl);
    return 0;
}

static gpointer
apteryx_node_copy_fn (gconstpointer src, gpointer data)
{
    return (gpointer) g_strdup ((const gchar *) src);
}

GList *search_result = NULL;
GList *
__wrap_apteryx_search (const char *path)
{
    GList *result = search_result;
    NP_ASSERT_NOT_NULL (result);
    search_result = NULL;
    return result;
}

GNode *apteryx_tree = NULL;
GNode *
__wrap_apteryx_get_tree (const char *path)
{
    GNode *tree = apteryx_tree;
    apteryx_tree = NULL;
    return tree;
}

bool
__wrap_apteryx_set_tree_full (GNode *root, uint64_t ts, bool wait_for_completion)
{
    g_assert_null (apteryx_tree);
    apteryx_tree = g_node_copy_deep (root, apteryx_node_copy_fn, NULL);
    return true;
}

char *apteryx_path;
char *apteryx_value;
bool
__wrap_apteryx_set_full (const char *path, const char *value, uint64_t ts,
                       bool wait_for_completion)
{
    g_assert_null (apteryx_path);
    g_assert_null (apteryx_value);
    apteryx_path = g_strdup (path);
    apteryx_value = g_strdup (value);
    return true;
}

char *apteryx_prune_path;
bool
__wrap_apteryx_prune (const char *path)
{
    g_assert_null (apteryx_prune_path);
    apteryx_prune_path = g_strdup (path);
    return true;
}

uint32_t procfs_uint32_t;
uint32_t
__wrap_procfs_read_uint32 (const char *path)
{
    return procfs_uint32_t;
}

char *procfs_string;
char *
__wrap_procfs_read_string (const char *path)
{
    return procfs_string;
}

#define ADD_TEST(fn) { \
    extern void fn (); \
    g_test_add_func ("/"#fn, fn); \
}

void
g_log_test_handler (const gchar *log_domain,
                       GLogLevelFlags log_level,
                       const gchar *message,
                       gpointer unused_data)
{
    switch (log_level & G_LOG_LEVEL_MASK) {
        case G_LOG_LEVEL_ERROR:
        case G_LOG_LEVEL_CRITICAL:
        case G_LOG_LEVEL_WARNING:
        case G_LOG_LEVEL_MESSAGE:
        case G_LOG_LEVEL_INFO:
            printf ("GLIB: %s\n", message);
            break;
        case G_LOG_LEVEL_DEBUG:
        default:
            break;
    }
    return;
}

int main (int argc, char *argv[])
{
    int rc;

    g_test_init (&argc, &argv, NULL);
    g_log_set_default_handler (g_log_test_handler, NULL);
    kermond_verbose = g_test_verbose ();

    ADD_TEST (test_entity_path_null);
    ADD_TEST (test_entity_invalid_path);
    ADD_TEST (test_entity_dynamic_ipv4_inconsistent_ifname);
    ADD_TEST (test_entity_new_dynamic_ipv4_no_interface);
    ADD_TEST (test_entity_new_dynamic_ipv4_no_address);
    ADD_TEST (test_entity_new_dynamic_ipv4);
    ADD_TEST (test_entity_del_dynamic_ipv4);
    ADD_TEST (test_entity_action_invalid);
    ADD_TEST (test_entity_addr_null);
    ADD_TEST (test_entity_addr_no_family);
    ADD_TEST (test_entity_addr_no_interface);
    ADD_TEST (test_entity_addr_no_address);
    ADD_TEST (test_entity_dynamic_ipv4_new_address);
    ADD_TEST (test_entity_dynamic_ipv4_del_address);
    ADD_TEST (test_entity_new_dynamic_ipv6_no_interface);
    ADD_TEST (test_entity_new_dynamic_ipv6_no_address);
    ADD_TEST (test_entity_new_dynamic_ipv6);
    ADD_TEST (test_entity_del_dynamic_ipv6);
    ADD_TEST (test_entity_dynamic_ipv6_new_address);
    ADD_TEST (test_entity_dynamic_ipv6_del_address);
    ADD_TEST (test_icmp4_path_null);
    ADD_TEST (test_icmp4_invalid_path);
    ADD_TEST (test_icmp4_invalid_parameter);
    ADD_TEST (test_icmpv4_dest_unreach_value_null);
    ADD_TEST (test_icmpv4_dest_unreach_enable);
    ADD_TEST (test_icmpv4_dest_unreach_disable);
    ADD_TEST (test_icmpv4_dest_unreach_value_invalid);
    ADD_TEST (test_icmpv4_error_ratelimit_value_null);
    ADD_TEST (test_icmpv4_error_ratelimit_value_0);
    ADD_TEST (test_icmpv4_error_ratelimit_value_2147483647);
    ADD_TEST (test_icmpv4_error_ratelimit_value_invalid);
    ADD_TEST (test_icmpv4_error_ratelimit_value_negative);
    ADD_TEST (test_icmpv4_error_ratelimit_value_too_high);
    ADD_TEST (test_icmp6_path_null);
    ADD_TEST (test_icmp6_invalid_path);
    ADD_TEST (test_icmp6_invalid_parameter);
    ADD_TEST (test_icmpv6_dest_unreach_value_null);
    ADD_TEST (test_icmpv6_dest_unreach_enable);
    ADD_TEST (test_icmpv6_dest_unreach_disable);
    ADD_TEST (test_icmpv6_dest_unreach_value_invalid);
    ADD_TEST (test_icmpv6_error_ratelimit_value_null);
    ADD_TEST (test_icmpv6_error_ratelimit_value_0);
    ADD_TEST (test_icmpv6_error_ratelimit_value_2147483647);
    ADD_TEST (test_icmpv6_error_ratelimit_value_invalid);
    ADD_TEST (test_icmpv6_error_ratelimit_value_negative);
    ADD_TEST (test_icmpv6_error_ratelimit_value_too_high);
    ADD_TEST (test_ifconfig_path_null);
    ADD_TEST (test_ifconfig_admin_value_null);
    ADD_TEST (test_ifconfig_admin_value_invalid);
    ADD_TEST (test_ifconfig_admin_down);
    ADD_TEST (test_ifconfig_admin_up);
    ADD_TEST (test_ifconfig_admin_inactive_down);
    ADD_TEST (test_ifconfig_admin_inactive_up);
    ADD_TEST (test_ifconfig_admin_to_active_default);
    ADD_TEST (test_ifconfig_admin_to_active_down);
    ADD_TEST (test_ifconfig_mtu_value_null);
    ADD_TEST (test_ifconfig_mtu_value_invalid);
    ADD_TEST (test_ifconfig_mtu_too_low);
    ADD_TEST (test_ifconfig_mtu_too_high);
    ADD_TEST (test_ifconfig_mtu_68);
    ADD_TEST (test_ifconfig_mtu_16535);
    ADD_TEST (test_ifconfig_mtu_inactive_default);
    ADD_TEST (test_ifconfig_mtu_inactive_1400);
    ADD_TEST (test_ifconfig_mtu_to_active_default);
    ADD_TEST (test_ifconfig_mtu_to_active_1400);
    ADD_TEST (test_ifstatus_action_invalid);
    ADD_TEST (test_ifstatus_link_null);
    ADD_TEST (test_ifstatus_link_incomplete);
    ADD_TEST (test_ifstatus_link_new);
    ADD_TEST (test_ifstatus_link_alias);
    ADD_TEST (test_ifstatus_link_del);
    ADD_TEST (test_ifstatus_admin_status_default_down);
    ADD_TEST (test_ifstatus_admin_status_up);
    ADD_TEST (test_ifstatus_oper_status);
    ADD_TEST (test_ifstatus_flags);
    ADD_TEST (test_ifstatus_phys_address);
    ADD_TEST (test_ifstatus_promisc_default_off);
    ADD_TEST (test_ifstatus_promisc_on);
    ADD_TEST (test_ifstatus_qdisc);
    ADD_TEST (test_ifstatus_mtu_default_1500);
    ADD_TEST (test_ifstatus_mtu_68);
    ADD_TEST (test_ifstatus_mtu_16535);
    ADD_TEST (test_ifstatus_speed_default);
    ADD_TEST (test_ifstatus_speed_1000);
    ADD_TEST (test_ifstatus_duplex_default_auto);
    ADD_TEST (test_ifstatus_duplex_full);
    ADD_TEST (test_ifstatus_duplex_half);
    ADD_TEST (test_ifstatus_arptype_default_void);
    ADD_TEST (test_ifstatus_arptype_ethernet);
    ADD_TEST (test_ifstatus_rxq_default_1);
    ADD_TEST (test_ifstatus_rxq_2);
    ADD_TEST (test_ifstatus_txqlen_default_1000);
    ADD_TEST (test_ifstatus_txqlen_2000);
    ADD_TEST (test_ifstatus_txq_default_1);
    ADD_TEST (test_ifstatus_txq_2);
    ADD_TEST (test_address_invalid);
    ADD_TEST (test_address_null);
    ADD_TEST (test_address_incomplete);
    ADD_TEST (test_address_ipv4);
    ADD_TEST (test_address_ipv6);
    ADD_TEST (test_static_addr4_path_null);
    ADD_TEST (test_static_addr4_path_invalid);
    ADD_TEST (test_static_addr4_ip_invalid);
    ADD_TEST (test_static_addr4_prefixlen_invalid);
    ADD_TEST (test_static_addr4_add_interface_inactive);
    ADD_TEST (test_static_addr4_add);
    ADD_TEST (test_static_addr4_add_interface_go_active);
    ADD_TEST (test_static_addr4_delete_interface_inactive);
    ADD_TEST (test_static_addr4_delete);
    ADD_TEST (test_static_addr6_path_null);
    ADD_TEST (test_static_addr6_path_invalid);
    ADD_TEST (test_static_addr6_ip_invalid);
    ADD_TEST (test_static_addr6_prefixlen_invalid);
    ADD_TEST (test_static_addr6_add_interface_inactive);
    ADD_TEST (test_static_addr6_add);
    ADD_TEST (test_static_addr6_add_interface_go_active);
    ADD_TEST (test_static_addr6_delete_interface_inactive);
    ADD_TEST (test_static_addr6_delete);
    ADD_TEST (test_neighbor_invalid);
    ADD_TEST (test_neighbor_null);
    ADD_TEST (test_neighbor_incomplete);
    ADD_TEST (test_neighbor_ipv4);
    ADD_TEST (test_neighbor_ipv6);
    ADD_TEST (test_static_neighbor4_path_null);
    ADD_TEST (test_static_neighbor4_path_invalid);
    ADD_TEST (test_static_neighbor4_lladdr_invalid);
    ADD_TEST (test_static_neighbor4_ip_invalid);
    ADD_TEST (test_static_neighbor4_add_interface_inactive);
    ADD_TEST (test_static_neighbor4_add);
    ADD_TEST (test_static_neighbor4_add_interface_go_active);
    ADD_TEST (test_static_neighbor4_delete_interface_inactive);
    ADD_TEST (test_static_neighbor4_delete);
    ADD_TEST (test_static_neighbor6_path_null);
    ADD_TEST (test_static_neighbor6_path_invalid);
    ADD_TEST (test_static_neighbor6_lladdr_invalid);
    ADD_TEST (test_static_neighbor6_ip_invalid);
    ADD_TEST (test_static_neighbor6_add_interface_inactive);
    ADD_TEST (test_static_neighbor6_add);
    ADD_TEST (test_static_neighbor6_add_interface_go_active);
    ADD_TEST (test_static_neighbor6_delete_interface_inactive);
    ADD_TEST (test_static_neighbor6_delete);
    ADD_TEST (test_neigh_path_null);
    ADD_TEST (test_neigh_invalid_path);
    ADD_TEST (test_neigh_invalid_parameter);
    ADD_TEST (test_neigh_opp_nd_value_null);
    ADD_TEST (test_neigh_opp_nd_value_disable);
    ADD_TEST (test_neigh_opp_nd_value_enable);
    ADD_TEST (test_neigh_opp_nd_value_invalid);
    ADD_TEST (test_neigh_aging_timeout_value_null);
    ADD_TEST (test_neigh_aging_timeout_value_0);
    ADD_TEST (test_neigh_aging_timeout_value_432000);
    ADD_TEST (test_neigh_aging_timeout_value_invalid);
    ADD_TEST (test_neigh_aging_timeout_value_too_low);
    ADD_TEST (test_neigh_aging_timeout_value_too_high);
    ADD_TEST (test_neigh_mac_disparity_value_null);
    ADD_TEST (test_neigh_mac_disparity_value_disable);
    ADD_TEST (test_neigh_mac_disparity_value_enable);
    ADD_TEST (test_neigh_mac_disparity_value_invalid);
    ADD_TEST (test_neigh_proxy_arp_value_null);
    ADD_TEST (test_neigh_proxy_arp_value_disable);
    ADD_TEST (test_neigh_proxy_arp_value_enable);
    ADD_TEST (test_neigh_proxy_arp_value_local);
    ADD_TEST (test_neigh_proxy_arp_value_both);
    ADD_TEST (test_neigh_proxy_arp_value_invalid);
    ADD_TEST (test_neigh_interface_opp_nd_value_null);
    ADD_TEST (test_neigh_interface_opp_nd_value_disable);
    ADD_TEST (test_neigh_interface_opp_nd_value_enable);
    ADD_TEST (test_neigh_interface_opp_nd_value_invalid);
    ADD_TEST (test_neighv6_path_null);
    ADD_TEST (test_neighv6_invalid_path);
    ADD_TEST (test_neighv6_invalid_parameter);
    ADD_TEST (test_neighv6_opp_nd_value_null);
    ADD_TEST (test_neighv6_opp_nd_value_disable);
    ADD_TEST (test_neighv6_opp_nd_value_enable);
    ADD_TEST (test_neighv6_opp_nd_value_invalid);
    ADD_TEST (test_tcp_path_null);
    ADD_TEST (test_tcp_invalid_path);
    ADD_TEST (test_tcp_invalid_parameter);
    ADD_TEST (test_tcp_synack_value_null);
    ADD_TEST (test_tcp_synack_value_0);
    ADD_TEST (test_tcp_synack_value_255);
    ADD_TEST (test_tcp_synack_value_invalid);
    ADD_TEST (test_tcp_synack_value_negative);
    ADD_TEST (test_tcp_synack_value_too_high);

    return g_test_run();
}

