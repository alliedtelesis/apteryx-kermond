/**
 * @file neighbor-cache.c
 * Kernel neighbor cache
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
#include <netlink/route/neighbour.h>
#include <netlink/route/link.h>
#include "ietf-ip.h"

/* Fallback if we have no link cache */
extern char *if_indextoname (unsigned int ifindex, char *ifname);

/* Required caches */
static struct nl_cache *neigh_cache = NULL;
static struct nl_cache *link_cache = NULL;

/**
 * Convert a Netlink neighbor object to an Apteryx tree
 * @param link Netlink neighbor object
 * @return the constructed tree
 */
static GNode *
neighbor_to_apteryx (struct rtnl_neigh *rn)
{
    char dst[INET6_ADDRSTRLEN + 5];
    char lladdr[INET6_ADDRSTRLEN + 5];
    char ifname[IFNAMSIZ] = {};
    int state;
    unsigned int flags;
    GNode *root;

    /* Parse */
    nl_addr2str (rtnl_neigh_get_lladdr (rn), lladdr, sizeof (lladdr));
    nl_addr2str (rtnl_neigh_get_dst (rn), dst, sizeof (dst));
    if (link_cache)
        rtnl_link_i2name (link_cache, rtnl_neigh_get_ifindex (rn), ifname, sizeof (ifname));
    else
        if_indextoname (rtnl_neigh_get_ifindex (rn), ifname);
    state = rtnl_neigh_get_state (rn);
    flags = rtnl_neigh_get_flags (rn);

    /* Build tree */
    root = g_node_new (g_strdup_printf (
            INTERFACES_STATE_PATH"/%s/%s/%s",
            ifname,
            rtnl_neigh_get_family (rn) == AF_INET ?
                    INTERFACES_STATE_IPV4_NEIGHBOR :
                    INTERFACES_STATE_IPV6_NEIGHBOR,
            dst));
    if (rtnl_neigh_get_family (rn) == AF_INET)
    {
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV4_NEIGHBOR_IP), strdup (dst));
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV4_NEIGHBOR_LINK_LAYER_ADDRESS), strdup (lladdr));
        if (state == NUD_PERMANENT)
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN),
                    strdup (INTERFACES_STATE_IPV4_ADDRESS_ORIGIN_STATIC));
        else
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN),
                    strdup (INTERFACES_STATE_IPV4_NEIGHBOR_ORIGIN_DYNAMIC));
    }
    else
    {
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_IP), strdup (dst));
        APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_LINK_LAYER_ADDRESS), strdup (lladdr));
        if (state == NUD_PERMANENT)
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN),
                    strdup (INTERFACES_STATE_IPV6_ADDRESS_ORIGIN_STATIC));
        else
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_ORIGIN_DYNAMIC));
        if (flags & NTF_ROUTER)
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_IS_ROUTER),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_IS_ROUTER_TRUE));
        switch (rtnl_neigh_get_state (rn))
        {
        case NUD_INCOMPLETE:
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE_INCOMPLETE));
            break;
        case NUD_REACHABLE:
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE_REACHABLE));
            break;
        case NUD_STALE:
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE_STALE));
            break;
        case NUD_DELAY:
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE_DELAY));
            break;
        case NUD_PROBE:
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE),
                    strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE_PROBE));
            break;
        case NUD_FAILED:
        case NUD_NOARP:
        case NUD_PERMANENT:
        default:
            APTERYX_LEAF (root, strdup (INTERFACES_STATE_IPV6_NEIGHBOR_STATE),
                        g_strdup_printf ("%d", rtnl_neigh_get_state (rn)));
        }
    }

    return root;
}

/**
 * Handle changes in neighbors from the kernel
 * @param action NL_ACT_NEW, NL_ACT_DEL or NL_ACT_CHANGE
 * @param old_obj v2 callbacks provide a before neighbor object
 * @param new_obj v1/v2 callbacks provide the new object
 */
static void
nl_neighbor_cb (int action, struct nl_object *old_obj, struct nl_object *new_obj)
{
    struct rtnl_neigh *rn;

    /* Check action */
    if (action != NL_ACT_NEW && action != NL_ACT_DEL && action != NL_ACT_CHANGE)
    {
        ERROR ("NEIGHBOR-CACHE: invalid neighbor cb action:%d\n", action);
        return;
    }

    //TODO delta between old and new for new version of libnl
    if (old_obj && !new_obj)
        new_obj = old_obj;

    /* Check neighbor object */
    if (!new_obj)
    {
        ERROR ("NEIGHBOR-CACHE: missing neighbor object\n");
        return;
    }
    rn = (struct rtnl_neigh *) new_obj;

    /* Debug */
    VERBOSE ("NEIGHBOR-CACHE: %s cache entry\n", action == NL_ACT_NEW ? "NEW" :
             (action == NL_ACT_DEL ? "DEL" : "CHG"));
    if (kermond_verbose)
        nl_object_dump (new_obj, &netlink_dp);

    /* Check family */
    if (rtnl_neigh_get_family (rn) != AF_INET && rtnl_neigh_get_family (rn) != AF_INET6)
    {
        ERROR ("NEIGHBOR-CACHE: invalid neighbor family:%d\n", rtnl_neigh_get_family (rn));
        return;
    }

    /* Check expected fields */
    if (!rtnl_neigh_get_dst (rn))
    {
        ERROR ("NEIGHBOR-CACHE: missing dst\n");
        return;
    }

    /* Process action */
    if (action == NL_ACT_DEL)
    {
        char dst[INET6_ADDRSTRLEN + 5];
        char ifname[IFNAMSIZ];
        char *path;

        /* Parse addresses */
        nl_addr2str (rtnl_neigh_get_dst (rn), dst, sizeof (dst));
        if (link_cache)
            rtnl_link_i2name (link_cache, rtnl_neigh_get_ifindex (rn), ifname, sizeof (ifname));
        else
            if_indextoname (rtnl_neigh_get_ifindex (rn), ifname);

        /* Generate path */
        path = g_strdup_printf (INTERFACES_STATE_PATH"/%s/%s/%s",
                        ifname,
                        rtnl_neigh_get_family (rn) == AF_INET ?
                                INTERFACES_STATE_IPV4_NEIGHBOR :
                                INTERFACES_STATE_IPV6_NEIGHBOR,
                        dst);

        apteryx_prune (path);
        free (path);
    }
    else
    {
        /* Add/Update Apteryx */
        GNode *tree = neighbor_to_apteryx (rn);
        apteryx_set_tree (tree);
        apteryx_free_tree (tree);
    }
}

/**
 * Module initialisation
 * @return true on success, false otherwise
 */
static bool
neighbor_cache_init (void)
{
    DEBUG ("NEIGHBOR-CACHE: Initialising\n");

    /* Start with an empty cache */
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV4_NEIGHBOR);
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV6_NEIGHBOR);

    /* Configure Netlink */
    netlink_register ("route/neigh", nl_neighbor_cb);
    link_cache = nl_cache_mngt_require_safe ("route/link");
    neigh_cache = nl_cache_mngt_require_safe ("route/neigh");
    if (!neigh_cache)
    {
        FATAL ("NEIGHBOR-CACHE: Failed to connect to neighbor cache\n");
        return false;
    }
    return true;
}

/**
 * Module shutdown
 */
static void
neighbor_cache_exit ()
{
    DEBUG ("NEIGHBOR-CACHE: Exiting\n");

    /* Drop the cache references and unregister the callback */
    if (neigh_cache)
        nl_cache_put (neigh_cache);
    if (link_cache)
        nl_cache_put (link_cache);
    netlink_unregister ("route/neigh", nl_neighbor_cb);

    /* Clear out the cache */
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV4_NEIGHBOR);
    apteryx_prune (INTERFACES_STATE_PATH"/*/"INTERFACES_STATE_IPV6_NEIGHBOR);
}

MODULE_CREATE ("neighbor-cache", neighbor_cache_init, NULL, neighbor_cache_exit);
