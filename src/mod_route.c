/**
 * @file rib
 * Manage static routes
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
#include "kermond.h"
#include <netlink/route/route.h>
#include <netlink/route/link.h>
#include "at-route.h"

/* Fallback if we have no link cache */
extern unsigned int if_nametoindex (const char *ifname);

/* Socket for making configuration changes */
static struct nl_sock *sock = NULL;

/* Required caches */
static struct nl_cache *link_cache = NULL;

/* Keep an Apteryx cache for static routes */
static GHashTable *v4_static_routes = NULL;
static GHashTable *v6_static_routes = NULL;

static char *
route_to_string (struct rtnl_route *rt_route)
{
    struct nl_addr *dst_addr = rtnl_route_get_dst (rt_route);
    int dst_prefix_len = 0;
    struct rtnl_nexthop *nexthop = rtnl_route_nexthop_n (rt_route, 0);
    struct nl_addr *nexthop_addr = nexthop ? rtnl_route_nh_get_gateway (nexthop) : NULL;
    char dest_str_addr[64] = { };
    char nexthop_str_addr[64] = { };
    int family = AF_UNSPEC;
    int ifindex = 0;

    if (nexthop)
    {
        nexthop_addr = rtnl_route_nh_get_gateway (nexthop);
        if (nexthop_addr)
        {
            family = nl_addr_get_family (nexthop_addr);
        }
        ifindex = rtnl_route_nh_get_ifindex (nexthop);
    }
    if (family == AF_UNSPEC && dst_addr)
    {
        family = nl_addr_get_family (dst_addr);
    }

    if (dst_addr && nl_addr_get_len (dst_addr) > 0)
    {
        dst_prefix_len = nl_addr_get_prefixlen (dst_addr);
        nl_addr_set_prefixlen (dst_addr, 8 * nl_addr_get_len (dst_addr));
        nl_addr2str (dst_addr, dest_str_addr, sizeof (dest_str_addr));
        nl_addr_set_prefixlen (dst_addr, dst_prefix_len);
    }
    else if (family == AF_INET)
        g_strlcpy (dest_str_addr, "0.0.0.0", sizeof (dest_str_addr));
    else if (family == AF_INET6)
        g_strlcpy (dest_str_addr, "::", sizeof (dest_str_addr));
    if (nexthop_addr)
        nl_addr2str (nexthop_addr, nexthop_str_addr, sizeof (nexthop_str_addr));
    else if (family == AF_INET)
        g_strlcpy (nexthop_str_addr, "0.0.0.0", sizeof (nexthop_str_addr));
    else if (family == AF_INET6)
        g_strlcpy (nexthop_str_addr, "::", sizeof (nexthop_str_addr));

    return g_strdup_printf ("%s_%d_%s_%d_%d_%d",
                            dest_str_addr, dst_prefix_len, nexthop_str_addr,
                            ifindex, rtnl_route_get_protocol (rt_route),
                            rtnl_route_get_priority (rt_route));
}

static void
nl_route_cb (int action, struct nl_object *old_obj, struct nl_object *new_obj)
{
    struct rtnl_route *rt = (struct rtnl_route *) new_obj;
    char *route = route_to_string (rt);
    char *data = (action == NL_ACT_NEW) ? route : NULL;

    if (!route || (action != NL_ACT_NEW && action != NL_ACT_DEL))
    {
        ERROR ("FIB: invalid route cb action:%d route:%s\n", action, route);
        free (route);
        return;
    }

    /* Debug */
    DEBUG ("FIB: %s(%s)\n", action == NL_ACT_NEW ? "NEW" : "DEL", route);
    if (kermond_verbose)
        nl_object_dump (new_obj, &netlink_dp);

    /* Update Apteryx */
    if (rtnl_route_get_family (rt) == AF_INET)
        apteryx_set_string (ROUTING_IPV4_FIB, route, data);
    else
        apteryx_set_string (ROUTING_IPV6_FIB, route, data);

    free (route);
}

static bool
route_valid (struct rtnl_route *rr)
{
    if (!rtnl_route_get_dst (rr))
        return false;
    if (!rtnl_route_nexthop_n (rr, 0))
        return false;
    return true;
}

static bool
parse_parameter (struct rtnl_route *rr, int index,
                 const char *parameter, const char *value, bool * changed)
{
    int err;

    /* Default to no change */
    if (changed)
        *changed = false;

    /* Parse parameter */
    if (strcmp (parameter, ROUTING_IPV4_RIB_ID) == 0)
    {
        if (value && strtoull (value, NULL, 10) != index)
        {
            ERROR ("RIB: ID does not match index!\n");
            return false;
        }
    }
    else if (strcmp (parameter, ROUTING_IPV4_RIB_PREFIX) == 0)
    {
        struct nl_addr *old = rtnl_route_get_dst (rr);
        struct nl_addr *addr = NULL;

        if (!value)
        {
            ERROR ("RIB: Do not support deleting a routes prefix\n");
            return false;
        }
        err = nl_addr_parse (value, rtnl_route_get_family (rr), &addr);
        if (err < 0)
        {
            ERROR ("RIB: Unable to parse prefix \"%s\": %s\n", value, nl_geterror (err));
            return false;
        }
        if (changed && (!old || nl_addr_cmp (old, addr) != 0))
        {
            *changed = true;
        }
        rtnl_route_set_dst (rr, addr);
    }
    else if (strcmp (parameter, ROUTING_IPV4_RIB_NEXTHOP) == 0)
    {
        struct rtnl_nexthop *old = rtnl_route_nexthop_n (rr, 0);
        struct rtnl_nexthop *nh = NULL;

        if (value)
        {
            struct nl_addr *addr;
            err = nl_addr_parse (value, rtnl_route_get_family (rr), &addr);
            if (err < 0)
            {
                ERROR ("RIB: Unable to parse nexthop: %s\n", nl_geterror (err));
                return false;
            }
            nh = rtnl_route_nh_alloc ();
            rtnl_route_nh_set_gateway (nh, addr);
            nl_addr_put (addr);
        }
        if (changed)
        {
            if ((!old && nh) || (old && !nh) ||
                (rtnl_route_nh_compare (old, nh, ~0, 1) != 0))
                *changed = true;
        }
        if (old)
            rtnl_route_remove_nexthop (rr, old);
        if (nh)
            rtnl_route_add_nexthop (rr, nh);
    }
    else if (strcmp (parameter, ROUTING_IPV4_RIB_IFNAME) == 0)
    {
        struct rtnl_nexthop *old = rtnl_route_nexthop_n (rr, 0);
        struct rtnl_nexthop *nh = NULL;
        int ifindex = 0;

        if (value)
        {
            if (link_cache)
                ifindex = rtnl_link_name2i (link_cache, value);
            else
                ifindex = if_nametoindex (value);
            if (ifindex == 0)
            {
                ERROR ("RIB: Unable to parse ifname: %s\n", value);
                return FALSE;
            }
            nh = rtnl_route_nh_alloc ();
            rtnl_route_nh_set_ifindex (nh, ifindex);
        }
        if (changed)
        {
            if ((!old && nh) || (old && !nh) ||
                (rtnl_route_nh_compare (old, nh, ~0, 1) != 0))
                *changed = true;
        }
        if (old)
            rtnl_route_remove_nexthop (rr, old);
        if (nh)
            rtnl_route_add_nexthop (rr, nh);
    }
    else if (strcmp (parameter, ROUTING_IPV4_RIB_DISTANCE) == 0)
    {
        uint32_t distance = 0;
        uint32_t prio;

        if (value)
        {
            distance = strtoull (value, NULL, 10);
            if (distance < 1 || distance > 65536)
            {
                ERROR ("RIB: Invalid distance \"%s\"\n", value);
                return false;
            }
        }
        /* Distance is the most significant component of priority */
        prio = rtnl_route_get_priority (rr);
        prio &= ~(0xFFFF0000);
        prio |= (distance << 16);
        if (changed && prio != rtnl_route_get_priority (rr))
        {
            *changed = true;
        }
        rtnl_route_set_priority (rr, prio);
    }
    else if (strcmp (parameter, ROUTING_IPV4_RIB_METRIC) == 0)
    {
        uint32_t metric = 0;
        uint32_t prio;

        if (value)
        {
            metric = strtoull (value, NULL, 10);
            if (metric < 1 || metric > 65536)
            {
                ERROR ("RIB: Invalid metric \"%s\"\n", value);
                return false;
            }
        }
        /* Metric is the least significant component of priority */
        prio = rtnl_route_get_priority (rr);
        prio &= ~(0x0000FFFF);
        prio |= metric;
        if (changed && rtnl_route_get_priority (rr) != prio)
        {
            *changed = true;
        }
        rtnl_route_set_priority (rr, prio);
    }
    else if (strcmp (parameter, ROUTING_IPV4_RIB_PROTOCOL) == 0)
    {
        if (value && strcmp (value, "static") != 0)
        {
            ERROR ("RIB: Only static routes supported\n");
            return false;
        }
    }
    else
    {
        DEBUG ("RIB: Ignoring unsupported parameter \"%s\"\n", parameter);
        // ROUTING_IPV4_RIB_VRF_ID
        // ROUTING_IPV4_RIB_SNMP_ROUTE_TYPE
        // ROUTING_IPV4_RIB_DHCP_INTERFACE
    }
    return true;
}

static struct rtnl_route *
apteryx_to_route (int family, int index)
{
    struct rtnl_route *rr;
    char *path;
    GNode *root;
    GNode *node;

    /* Create Apteryx path */
    if (family == 4)
        path = g_strdup_printf (ROUTING_IPV4_RIB_PATH "/%d", index);
    else
        path = g_strdup_printf (ROUTING_IPV4_RIB_PATH "/%d", index);

    /* Get all configuration for this route */
    root = apteryx_get_tree (path);
    free (path);

    /* Create route with defaults */
    rr = rtnl_route_alloc ();
    rtnl_route_set_family (rr, family == 4 ? AF_INET : AF_INET6);
    rtnl_route_set_table (rr, RT_TABLE_MAIN);
    rtnl_route_set_type (rr, RTN_UNICAST);
    rtnl_route_set_scope (rr, 0);
    rtnl_route_set_priority (rr, 0);
    rtnl_route_set_protocol (rr, RTPROT_STATIC);

    /* Parse parameters */
    node = root ? g_node_first_child (root) : NULL;
    while (node)
    {
        /* Parse the Apteryx parameter */
        if (!parse_parameter (rr, index, APTERYX_NAME (node), APTERYX_VALUE (node), NULL))
        {
            rtnl_route_put (rr);
            return NULL;
        }
        node = node->next;
    }
    apteryx_free_tree (root);
    return rr;
}

static bool
route_add (int family, int index, struct rtnl_route *rr)
{
    int err;

    /* Debug */
    VERBOSE ("RIB: ADD static route\n");
    if (kermond_verbose)
        nl_object_dump ((struct nl_object *) rr, &netlink_dp);

    /* Add the route */
    if ((err = rtnl_route_add (sock, rr, NLM_F_EXCL)) < 0)
    {
        ERROR ("RIB: Unable to add route: %s\n", nl_geterror (err));
        return false;
    }

    /* Store the route by index */
    if (family == 4)
        g_hash_table_replace (v4_static_routes, GINT_TO_POINTER (index), rr);
    else
        g_hash_table_replace (v6_static_routes, GINT_TO_POINTER (index), rr);

    return true;
}

static void
route_del (int family, int index, struct rtnl_route *rr)
{
    int err;

    /* Debug */
    VERBOSE ("RIB: DEL static route\n");
    if (kermond_verbose)
        nl_object_dump ((struct nl_object *) rr, &netlink_dp);

    /* Delete the route */
    if ((err = rtnl_route_delete (sock, rr, 0)) < 0)
    {
        ERROR ("RIB: Unable to delete route: %s\n", nl_geterror (err));
    }

    /* Remove from the lookup by index */
    if (index != 0)
    {
        if (family == 4)
            g_hash_table_remove (v4_static_routes, GINT_TO_POINTER (index));
        else
            g_hash_table_remove (v6_static_routes, GINT_TO_POINTER (index));
    }
    return;
}

static bool
watch_static_routes (const char *path, const char *value)
{
    struct rtnl_route *rr = NULL;
    bool changed = false;
    char parameter[64];
    int family;
    int index;

    DEBUG ("RIB: %s = %s\n", path, value);

    /* Parse family, index and the parameter that has changed */
    if (sscanf (path, ROUTING_PATH "/ipv%d/rib/%d/%64s", &family, &index, parameter) != 3)
    {
        ERROR ("RIB: Invalid static route path (%s)\n", path);
        return false;
    }

    DEBUG ("RIB: family:%d index:%d parameter:%s\n", family, index, parameter);

    /* Find an existing route based on this ID */
    if (family == 4)
        rr = (struct rtnl_route *) g_hash_table_lookup (v4_static_routes,
                                                        GINT_TO_POINTER (index));
    else
        rr = (struct rtnl_route *) g_hash_table_lookup (v6_static_routes,
                                                        GINT_TO_POINTER (index));

    /* Add */
    if (!rr)
    {
        /* Create route from Apteryx */
        rr = apteryx_to_route (family, index);
        if (!rr || !route_valid (rr))
        {
            /* Probably not enough info yet */
            if (rr)
                rtnl_route_put (rr);
            VERBOSE ("RIB: Route configuration currently not valid\n");
            goto done;
        }

        /* Add the route */
        if (!route_add (family, index, rr))
        {
            rtnl_route_put (rr);
            ERROR ("RIB: Failed to add new route\n");
        }
    }
    /* Delete */
    else if (!value &&
             (strcmp (parameter, ROUTING_IPV4_RIB_ID) == 0 ||
              strcmp (parameter, ROUTING_IPV4_RIB_PREFIX) == 0))
    {
        route_del (family, index, rr);
        rtnl_route_put (rr);
    }
    /* Update */
    else
    {
        struct rtnl_route *old;

        /* Copy the old route */
        old = (struct rtnl_route *) nl_object_clone ((struct nl_object *) rr);

        /* Parse the Apteryx parameter */
        if (!parse_parameter (rr, index, parameter, value, &changed))
        {
            ERROR ("RIB: Invalid route configuration change\n");
            rtnl_route_put (old);
            goto done;
        }

        /* Check if we need to update the route */
        if (!changed)
        {
            VERBOSE ("RIB: No change to route configuration\n");
            rtnl_route_put (old);
            goto done;
        }

        // TODO replace rather than delete/add
        route_del (family, index, old);
        if (route_valid (rr))
        {
            route_add (family, index, rr);
        }
    }

  done:
    return true;
}

static void
exit_cb (gpointer key, gpointer value, gpointer user_data)
{
    route_del (GPOINTER_TO_INT (user_data), 0, (struct rtnl_route *) value);
}

static bool
route_init (void)
{
    int err;

    DEBUG ("ROUTE: Initialising\n");

    /* Local lookup cache */
    v4_static_routes = g_hash_table_new (g_direct_hash, g_direct_equal);
    v6_static_routes = g_hash_table_new (g_direct_hash, g_direct_equal);

    /* Setup Netlink socket for changing the kernel route database */
    sock = nl_socket_alloc ();
    if ((err = nl_connect (sock, NETLINK_ROUTE)) < 0)
    {
        FATAL ("RIB: Unable to connect socket: %s\n", nl_geterror (err));
        return false;
    }

    /* Get a handle to the link cache for interface to ifindex conversion */
    link_cache = nl_cache_mngt_require_safe ("route/link");

    /* Setup Apteryx */
    apteryx_prune (ROUTING_IPV4_FIB);
    apteryx_prune (ROUTING_IPV6_FIB);

    /* Setup Netlink */
    netlink_register ("route/route", nl_route_cb);

    return true;
}

static bool
route_start ()
{
    DEBUG ("ROUTE: Starting\n");

    /* Setup Apteryx watchers */
    apteryx_watch (ROUTING_IPV4_RIB_PATH "/*", watch_static_routes);
    apteryx_watch (ROUTING_IPV6_RIB_PATH "/*", watch_static_routes);

    /* Load existing configuration */
    apteryx_rewatch_tree (ROUTING_IPV4_RIB_PATH, watch_static_routes);
    apteryx_rewatch_tree (ROUTING_IPV4_RIB_PATH, watch_static_routes);

    return true;
}

static void
route_exit ()
{
    DEBUG ("ROUTE: Exiting\n");

    /* Remove Apteryx watchers */
    apteryx_unwatch (ROUTING_IPV4_RIB_PATH "/*", watch_static_routes);
    apteryx_unwatch (ROUTING_IPV6_RIB_PATH "/*", watch_static_routes);

    /* Delete any routes we added */
    g_hash_table_foreach (v4_static_routes, exit_cb, GINT_TO_POINTER (4));
    g_hash_table_destroy (v4_static_routes);
    g_hash_table_foreach (v6_static_routes, exit_cb, GINT_TO_POINTER (6));
    g_hash_table_destroy (v6_static_routes);

    /* Remove Netlink configuration */
    netlink_unregister ("route/route", nl_route_cb);

    /* Remove Netlink socket */
    if (sock)
        nl_close (sock);

    /* Detach from the link cache */
    if (link_cache)
        nl_cache_put (link_cache);

    /* Remove FIB from Apteryx */
    apteryx_prune (ROUTING_IPV4_FIB);
    apteryx_prune (ROUTING_IPV6_FIB);
}

MODULE_CREATE ("route", route_init, route_start, route_exit);
