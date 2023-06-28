/**
 * @file address-static.c
 * Manage static addresses
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
#include "ietf-interfaces.h"
#include "ietf-ip.h"

/* Fallback if we have no link cache */
extern unsigned int if_nametoindex (const char *ifname);

/* Required caches */
static struct nl_cache *link_cache = NULL;

/* Socket for making configuration changes */
static struct nl_sock *sock = NULL;

/**
 * Convert an Apteryx neighbor into a Netlink neighbor
 * @param path path that includes the IP and interface
 * @param lladdr mac address for the static neighbor
 * @return true if the callback was expected
 */
static struct rtnl_addr *
apteryx_to_address (const char *path, const char *lladdr)
{
    struct rtnl_addr *ra;
    struct nl_addr *addr;
    char ip[64];
    char ifname[64];
    char parameter[64];
    int family;
    int ifindex;
    int err;

    /* Parse family, ip and iface */
    if (!path ||
        sscanf (path, INTERFACES_PATH "/%64[^/]/ipv%d/address/%64[^/]/%64s",
                ifname, &family, ip, parameter) != 4)
    {
        ERROR ("ADDRESS: Invalid static address: %s = %s\n", path, lladdr);
        return NULL;
    }

    /* Currently only process ip parameter */
    if (strcmp (parameter, INTERFACES_STATE_INTERFACE_IPV6_ADDRESS_IP) != 0)
    {
        return NULL;
    }

    /* Create a new address */
    ra = rtnl_addr_alloc ();
    rtnl_addr_set_family (ra, family == 4 ? AF_INET : AF_INET6);

    /* Parse ip */
    err = nl_addr_parse (ip, rtnl_addr_get_family (ra), &addr);
    if (err < 0)
    {
        ERROR ("ADDRESS: Unable to parse ip: %s\n", nl_geterror (err));
        rtnl_addr_put (ra);
        return NULL;
    }
    rtnl_addr_set_local (ra, addr);
    nl_addr_put (addr);

    /* Parse interface */
    if (link_cache)
        ifindex = rtnl_link_name2i (link_cache, ifname);
    else
        ifindex = if_nametoindex (ifname);
    if (ifindex == 0)
    {
        DEBUG ("ADDRESS: Link \"%s\" is not currently active\n", ifname);
        rtnl_addr_put (ra);
        return NULL;
    }
    rtnl_addr_set_ifindex (ra, ifindex);

    return ra;
}

/**
 * Manage changes in static address configuration
 * @param path path to configuration setting that has changed
 * @param value new value for the specified path
 * @return true if callback was expected
 */
static bool
apteryx_static_address_cb (const char *path, const char *value)
{
    struct rtnl_addr *ra;
    int err;

    /* Parse family, ip and interface from path */
    ra = apteryx_to_address (path, value);
    if (!ra)
    {
        /* Not (currently) valid for some reason */
        return true;
    }

    /* Debug */
    VERBOSE ("ADDRESS: %s static address\n", value ? "NEW" : "DEL");
    if (kermond_verbose)
        nl_object_dump ((struct nl_object *) ra, &netlink_dp);

    /* Add */
    if (value)
    {
        /* Add the address */
        if ((err = rtnl_addr_add (sock, ra, NLM_F_REPLACE | NLM_F_CREATE)) < 0)
        {
            ERROR ("NEIGHBOR: Unable to add address: %s", nl_geterror (err));
        }
    }
    /* Delete */
    else
    {
        /* Delete the address */
        if ((err = rtnl_addr_delete (sock, ra, 0)) < 0)
        {
            ERROR ("NEIGHBOR: Unable to delete address: %s\n", nl_geterror (err));
        }
    }

    rtnl_addr_put (ra);
    return true;
}

/**
 * Netlink callback to apply configuration to new interfaces
 * @param action NL_ACT_NEW only used
 * @param old_obj v2 callbacks provide a before link object
 * @param new_obj v1/v2 callbacks
 */
static void
nl_if_cb (int action, struct nl_object *old_obj, struct nl_object *new_obj)
{
    struct rtnl_link *link = (struct rtnl_link *) new_obj;
    GList *paths, *iter;
    char *path;

    /* We only care about new interfaces */
    if (action != NL_ACT_NEW)
    {
        return;
    }

    //TODO delta between old and new for new version of libnl
    if (old_obj && !new_obj)
        new_obj = old_obj;

    /* Find all static IPv4 addresses with this interface */
    path = g_strdup_printf (INTERFACES_PATH"/%s"INTERFACES_INTERFACE_IPV4_ADDRESS_PATH"/",
            rtnl_link_get_name (link));
    paths = apteryx_search (path);
    g_free (path);
    for (iter = g_list_first (paths); iter; iter = g_list_next (iter))
    {
        /* Add this address */
        apteryx_rewatch_tree ((char *)iter->data, apteryx_static_address_cb);
    }
    g_list_free_full (paths, free);
}

/**
 * Module initialisation
 * @return true on success, false otherwise
 */
static bool
static_address_init (void)
{
    int err;

    DEBUG ("STATIC-ADDRESS: Initialising\n");

    /* Configure Netlink */
    link_cache = nl_cache_mngt_require_safe ("route/link");
    netlink_register ("route/link", nl_if_cb);
    sock = nl_socket_alloc ();
    if ((err = nl_connect (sock, NETLINK_ROUTE)) < 0)
    {
        FATAL ("STATIC-ADDRESS: Unable to connect socket: %s\n", nl_geterror (err));
        return false;
    }
    return true;
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
static_address_start ()
{
    DEBUG ("STATIC-ADDRESS: Starting\n");

    /* Setup Apteryx */
    apteryx_watch (INTERFACES_PATH"/*/"INTERFACES_INTERFACE_IPV4_ADDRESS_PATH,
            apteryx_static_address_cb);
    apteryx_watch (INTERFACES_PATH"/*/"INTERFACES_INTERFACE_IPV6_ADDRESS_PATH,
            apteryx_static_address_cb);

    /* Load existing configuration */
    apteryx_rewatch_tree (INTERFACES_PATH"/*/"INTERFACES_INTERFACE_IPV4_ADDRESS_PATH,
            apteryx_static_address_cb);
    apteryx_rewatch_tree (INTERFACES_PATH"/*/"INTERFACES_INTERFACE_IPV6_ADDRESS_PATH,
            apteryx_static_address_cb);

    return true;
}

/**
 * Module shutdown
 */
static void
static_address_exit ()
{
    DEBUG ("STATIC-ADDRESS: Exiting\n");

    /* Unconfigure Apteryx */
    apteryx_unwatch (INTERFACES_PATH"/*/"INTERFACES_INTERFACE_IPV4_ADDRESS_PATH,
            apteryx_static_address_cb);
    apteryx_unwatch (INTERFACES_PATH"/*/"INTERFACES_INTERFACE_IPV6_ADDRESS_PATH,
            apteryx_static_address_cb);

    /* Remove Netlink interface */
    if (sock)
        nl_close (sock);
    if (link_cache)
        nl_cache_put (link_cache);
    netlink_unregister ("route/link", nl_if_cb);
}

MODULE_CREATE ("static-address", static_address_init, static_address_start,
               static_address_exit);
