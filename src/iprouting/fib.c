/**
 * @file fib.c
 * Monitor kernel routes
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
#include "iprouting.h"

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
fib_init (void)
{
    DEBUG ("FIB: Initialising\n");

    /* Setup Apteryx */
    apteryx_prune (ROUTING_IPV4_FIB);
    apteryx_prune (ROUTING_IPV6_FIB);

    /* Setup Netlink */
    netlink_register ("route/route", nl_route_cb);

    return true;
}

static void
fib_exit ()
{
    DEBUG ("FIB: Exiting\n");

    /* Remove Netlink configuration */
    netlink_unregister ("route/route", nl_route_cb);

    /* Remove FIB from Apteryx */
    apteryx_prune (ROUTING_IPV4_FIB);
    apteryx_prune (ROUTING_IPV6_FIB);
}

MODULE_CREATE ("fib", fib_init, NULL, fib_exit);
