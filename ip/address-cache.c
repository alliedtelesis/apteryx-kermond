/**
 * @file address-cache.c
 * Kernel address cache
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
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <arpa/inet.h>
#include "ietf-ip.h"

/* Fallback if we have no link cache */
extern char *if_indextoname (unsigned int ifindex, char *ifname);

/* Required caches */
static struct nl_cache *addr_cache = NULL;
static struct nl_cache *link_cache = NULL;

/**
 * Convert a Netlink address object to an Apteryx tree
 * @param link Netlink address object
 * @return the constructed tree
 */
static GNode *
address_to_apteryx (struct rtnl_addr *ra)
{
    char ip[INET6_ADDRSTRLEN + 5];
    char ifname[IFNAMSIZ] = {};
    int prefixlen;
    GNode *root;
    GNode *node;

    /* Parse */
    inet_ntop (rtnl_addr_get_family (ra),
            nl_addr_get_binary_addr (rtnl_addr_get_local (ra)), ip, sizeof (ip));
    if (link_cache)
        rtnl_link_i2name (link_cache, rtnl_addr_get_ifindex (ra), ifname, sizeof (ifname));
    else
        if_indextoname (rtnl_addr_get_ifindex (ra), ifname);
    prefixlen = rtnl_addr_get_prefixlen (ra);

    /* Build tree */
    root = g_node_new (g_strdup_printf (
            INTERFACES_STATE_PATH"/%s/%s/%s",
            ifname,
            rtnl_addr_get_family (ra) == AF_INET ?
                    INTERFACES_STATE_IPV4_ADDRESS :
                    INTERFACES_STATE_IPV6_ADDRESS,
                    ip));
    if (rtnl_addr_get_family (ra) == AF_INET)
    {
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV4_ADDRESS_IP), strdup (ip));
        node = APTERYX_NODE (root, strdup (INTERFACES_STATE_IPV4_ADDRESS_SUBNET));
        APTERYX_LEAF_INT (node, "prefix-length", prefixlen);
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN),
                            strdup (INTERFACES_STATE_IPV4_ADDRESS_ORIGIN_OTHER));
    }
    else
    {
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_ADDRESS_IP), strdup (ip));
        APTERYX_LEAF_INT (root, INTERFACES_STATE_IPV6_ADDRESS_PREFIX_LENGTH, prefixlen);
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_ADDRESS_ORIGIN),
                            strdup (INTERFACES_STATE_IPV6_ADDRESS_ORIGIN_OTHER));
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_ADDRESS_STATUS),
                            strdup (INTERFACES_STATE_IPV6_ADDRESS_STATUS_UNKNOWN));
    }

    return root;
}

/**
 * Handle changes in addresses from the kernel
 * @param action NL_ACT_NEW, NL_ACT_DEL or NL_ACT_CHANGE
 * @param old_obj v2 callbacks provide a before neighbor object
 * @param new_obj v1/v2 callbacks provide the new object
 */
static void
nl_address_cb (int action, struct nl_object *old_obj, struct nl_object *new_obj)
{
    struct rtnl_addr *ra;

    /* Check action */
    if (action != NL_ACT_NEW && action != NL_ACT_DEL && action != NL_ACT_CHANGE)
    {
        ERROR ("ADDRESS-CACHE: invalid address cb action:%d\n", action);
        return;
    }

    //TODO delta between old and new for new version of libnl
    if (old_obj && !new_obj)
        new_obj = old_obj;

    /* Check neighbor object */
    if (!new_obj)
    {
        ERROR ("ADDRESS-CACHE: missing address object\n");
        return;
    }
    ra = (struct rtnl_addr *) new_obj;

    /* Debug */
    VERBOSE ("ADDRESS-CACHE: %s cache entry\n", action == NL_ACT_NEW ? "NEW" :
             (action == NL_ACT_DEL ? "DEL" : "CHG"));
    if (kermond_verbose)
        nl_object_dump (new_obj, &netlink_dp);

    /* Check family */
    if (rtnl_addr_get_family (ra) != AF_INET && rtnl_addr_get_family (ra) != AF_INET6)
    {
        ERROR ("ADDRESS-CACHE: invalid address family:%d\n", rtnl_addr_get_family (ra));
        return;
    }

    /* Check expected fields */
    if (!rtnl_addr_get_local (ra))
    {
        ERROR ("ADDRESS-CACHE: missing address\n");
        return;
    }

    /* Process action */
    if (action == NL_ACT_DEL)
    {
        char ip[INET6_ADDRSTRLEN + 5];
        char ifname[IFNAMSIZ];
        char *path;

        /* Parse addresses */
        nl_addr2str (rtnl_addr_get_local (ra), ip, sizeof (ip));
        if (link_cache)
            rtnl_link_i2name (link_cache, rtnl_addr_get_ifindex (ra), ifname, sizeof (ifname));
        else
            if_indextoname (rtnl_addr_get_ifindex (ra), ifname);

        /* Generate path */
        path = g_strdup_printf (INTERFACES_STATE_PATH"/%s/%s/%s",
                        ifname,
                        rtnl_addr_get_family (ra) == AF_INET ?
                                INTERFACES_STATE_IPV4_ADDRESS :
                                INTERFACES_STATE_IPV6_ADDRESS,
                        ip);

        apteryx_prune (path);
        free (path);
    }
    else
    {
        /* Add/Update Apteryx */
        GNode *tree = address_to_apteryx (ra);
        apteryx_set_tree (tree);
        apteryx_free_tree (tree);
    }
}

/**
 * Module initialisation
 * @return true on success, false otherwise
 */
static bool
address_cache_init (void)
{
    DEBUG ("ADDRESS-CACHE: Initialising\n");

    /* Start with an empty cache */
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV4_ADDRESS);
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV6_ADDRESS);

    /* Configure Netlink */
    netlink_register ("route/addr", nl_address_cb);
    link_cache = nl_cache_mngt_require_safe ("route/link");
    addr_cache = nl_cache_mngt_require_safe ("route/addr");
    if (!addr_cache)
    {
        FATAL ("ADDRESS-CACHE: Failed to connect to address cache\n");
        return false;
    }
    return true;
}

/**
 * Module shutdown
 */
static void
address_cache_exit ()
{
    DEBUG ("ADDRESS-CACHE: Exiting\n");

    /* Drop the cache references and unregister the callback */
    if (addr_cache)
        nl_cache_put (addr_cache);
    if (link_cache)
        nl_cache_put (link_cache);
    netlink_unregister ("route/addr", nl_address_cb);

    /* Clear out the cache */
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV4_ADDRESS);
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV6_ADDRESS);
}

MODULE_CREATE ("address-cache", address_cache_init, NULL, address_cache_exit);
