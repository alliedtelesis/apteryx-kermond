/**
 * @file ifconfig.c
 * Manage kernel interfaces
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
#include <netlink/route/link.h>
#include "interface.h"

/* Required caches */
static struct nl_cache *link_cache = NULL;

/* Netlink socket for making configuration changes */
static struct nl_sock *sock = NULL;

/**
 * Callback for changes to interface settings in Apteryx
 * @param path the apteryx path (/interface/interfaces/<ifname>/<parameter>)
 * @param value changed value for the specified parameter
 * @return true if we expected this callback, false otherwise
 */
static bool
apteryx_if_cb (const char *path, const char *value)
{
    struct rtnl_link *link = NULL;
    struct rtnl_link *change;
    char ifname[64];
    char parameter[64];
    int err;

    /* Parse family, index and the parameter that has changed */
    if (!path || sscanf (path, INTERFACE_INTERFACES_PATH "/%64[^/]/"
                INTERFACE_INTERFACES_SETTINGS_PATH "/%64s", ifname, parameter) != 2)
    {
        ERROR ("IFCONFIG: Invalid interface settings path (%s)\n", path);
        return false;
    }

    /* Find link in the link cache */
    if (link_cache)
        link = rtnl_link_get_by_name (link_cache, ifname);
    if (!link)
    {
        DEBUG ("IFCONFIG: Link \"%s\" is not currently active\n", ifname);
        return true;
    }

    /* Create a link object to add the changes to */
    change = rtnl_link_alloc ();

    /* Admin status */
    if (strcmp (parameter, "admin-status") == 0)
    {
        int status = INTERFACE_INTERFACES_SETTINGS_ADMIN_STATUS_DEFAULT;
        if ((value && sscanf (value, "%d", &status) != 1) ||
            (status != INTERFACE_INTERFACES_SETTINGS_ADMIN_STATUS_ADMIN_DOWN &&
             status != INTERFACE_INTERFACES_SETTINGS_ADMIN_STATUS_ADMIN_UP))
        {
            status = INTERFACE_INTERFACES_SETTINGS_ADMIN_STATUS_DEFAULT;
            ERROR ("IFCONFIG: Invalid admin-status (%s) using default (%d)\n",
                   value, status);
        }
        if (status == INTERFACE_INTERFACES_SETTINGS_ADMIN_STATUS_ADMIN_UP)
            rtnl_link_set_flags (change, IFF_UP);
        else
            rtnl_link_unset_flags (change, IFF_UP);
    }
    /* MTU */
    else if (strcmp (parameter, "mtu") == 0)
    {
        int mtu = 1500;
        if ((value && sscanf (value, "%d", &mtu) != 1) || mtu < 68 || mtu > 16535)
        {
            mtu = 1500;
            ERROR ("IFCONFIG: Invalid MTU (%s) using default (%d)\n", value, mtu);
        }
        rtnl_link_set_mtu (change, mtu);
    }
    else
    {
        DEBUG ("IFCONFIG: Unexpected \"%s\" setting \"%s\"\n", ifname, parameter);
        rtnl_link_put (change);
        return true;
    }

    /* Debug */
    VERBOSE ("IFCONFIG: Update %s\n", ifname);
    if (kermond_verbose)
        nl_object_dump ((struct nl_object *) change, &netlink_dp);

    /* Make the change */
    if ((err = rtnl_link_change (sock, link, change, 0)) < 0)
    {
        ERROR ("IFCONFIG: Unable to update link: %s", nl_geterror (err));
    }

    rtnl_link_put (change);
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
    char *path;

    /* We only care about new interfaces */
    if (action != NL_ACT_NEW)
    {
        return;
    }

    //TODO delta between old and new for new version of libnl
    if (old_obj && !new_obj)
        new_obj = old_obj;

    /* Load all configuration for this interface */
    path = g_strdup_printf (INTERFACE_INTERFACES_PATH "/%s/"
                            INTERFACE_INTERFACES_SETTINGS_PATH, rtnl_link_get_name (link));
    apteryx_rewatch_tree (path, apteryx_if_cb);
    free (path);
}

/**
 * Module initialisation
 * @return true on success, false otherwise
 */
static bool
ifconfig_init (void)
{
    int err;

    DEBUG ("IFCONFIG: Initialising\n");

    /* Create the link cache and register for callbacks */
    netlink_register ("route/link", nl_if_cb);
    link_cache = nl_cache_mngt_require_safe ("route/link");
    if (!link_cache)
    {
        FATAL ("IFCONFIG: Failed to connect to link cache\n");
        return false;
    }

    /* Allocate a Netlink socket for making configuration changes */
    sock = nl_socket_alloc ();
    if ((err = nl_connect (sock, NETLINK_ROUTE)) < 0)
    {
        FATAL ("IFCONFIG: Unable to connect socket: %s\n", nl_geterror (err));
        return false;
    }

    return true;
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
ifconfig_start ()
{
    DEBUG ("IFCONFIG: Starting\n");

    /* Setup Apteryx watchers */
    apteryx_watch (INTERFACE_INTERFACES_PATH "/*/"
                   INTERFACE_INTERFACES_SETTINGS_PATH "/*", apteryx_if_cb);

    /* Load existing configuration */
    GList *iflist = apteryx_search (INTERFACE_INTERFACES_PATH "/");
    for (GList * iter = iflist; iter; iter = iter->next)
    {
        char *path = g_strdup_printf ("%s/" INTERFACE_INTERFACES_SETTINGS_PATH,
                                      (char *) iter->data);
        apteryx_rewatch_tree (path, apteryx_if_cb);
        free (path);
    }
    g_list_free_full (iflist, free);

    return true;
}

/**
 * Module shutdown
 */
static void
ifconfig_exit ()
{
    DEBUG ("IFCONFIG: Exiting\n");

    /* Detach Apteryx watchers */
    apteryx_unwatch (INTERFACE_INTERFACES_PATH "/*/"
                     INTERFACE_INTERFACES_SETTINGS_PATH "/*", apteryx_if_cb);

    /* Detach our callback and unref the link cache */
    if (sock)
        nl_close (sock);
    if (link_cache)
        nl_cache_put (link_cache);
    netlink_unregister ("route/link", nl_if_cb);
}

MODULE_CREATE ("ifconfig", ifconfig_init, ifconfig_start, ifconfig_exit);
