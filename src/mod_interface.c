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
#include "at-interface.h"

/* Required caches */
static struct nl_cache *link_cache = NULL;

/* Netlink socket for making configuration changes */
static struct nl_sock *sock = NULL;

/**
 * Retrieve the interface speed from the kernel
 * @param name interface name
 * @return the interface speed
 */
static unsigned int
if_speed_get (char *name)
{
    uint32_t speed = 0;
    char *file_name;

    file_name = g_strdup_printf ("/sys/class/net/%s/speed", name);
    speed = procfs_read_uint32 (file_name);
    free (file_name);
    if ((int) speed == -1)
    {
        speed = 0;
    }
    return speed;
}

/**
 * Retrieve the interface duplex setting from the kernel
 * @param name interface name
 * @return the interface duplex
 */
static unsigned int
if_duplex_get (char *name)
{
    unsigned int duplex = INTERFACE_INTERFACES_STATUS_DUPLEX_AUTO;
    char *file_name = NULL;

    file_name = g_strdup_printf ("/sys/class/net/%s/duplex", name);
    char *sduplex = procfs_read_string (file_name);
    if (sduplex && strcmp (sduplex, "half") == 0)
    {
        duplex = INTERFACE_INTERFACES_STATUS_DUPLEX_HALF;
    }
    else if (sduplex && strcmp (sduplex, "full") == 0)
    {
        duplex = INTERFACE_INTERFACES_STATUS_DUPLEX_FULL;
    }
    free (file_name);
    return duplex;
}

/**
 * Convert a Netlink link object to an Apteryx tree for interface status
 * @param link Netlink link object
 * @return the constructed tree
 */
static GNode *
link_to_apteryx (struct rtnl_link *link)
{
    char phys_address[128];
    GNode *root, *ifalias, *node, *status;

    /* Minimum requirements */
    if (!link || !rtnl_link_get_name (link) || !rtnl_link_get_ifindex (link))
    {
        ERROR ("IFSTATUS: invalid link object\n");
        return NULL;
    }

    /* Build tree */
    root = g_node_new (strdup ("/"));
    ifalias = apteryx_path_to_node (root, INTERFACE_IF_ALIAS, NULL);
    APTERYX_LEAF (ifalias, g_strdup_printf ("%d", rtnl_link_get_ifindex (link)),
                  strdup (rtnl_link_get_name (link)));
    node = apteryx_path_to_node (root, INTERFACE_INTERFACES_PATH, NULL);
    node = APTERYX_NODE (node, strdup (rtnl_link_get_name (link)));
    /* Name */
    APTERYX_LEAF (node, strdup (INTERFACE_INTERFACES_NAME),
                  strdup (rtnl_link_get_name (link)));
    /* if-index */
    APTERYX_LEAF (node, strdup (INTERFACE_INTERFACES_IF_INDEX),
                  g_strdup_printf ("%d", rtnl_link_get_ifindex (link)));
    /* L3 */
    if (rtnl_link_get_master (link))
    {
        APTERYX_LEAF (node, strdup (INTERFACE_INTERFACES_L3),
                      g_strdup_printf ("%d", INTERFACE_INTERFACES_L3_DEFAULT));
    }
    else
    {
        APTERYX_LEAF (node, strdup (INTERFACE_INTERFACES_L3),
                      g_strdup_printf ("%d", INTERFACE_INTERFACES_L3_L3_IF));
    }
    /* Status */
    status = APTERYX_NODE (node, strdup (INTERFACE_INTERFACES_STATUS_PATH));
    APTERYX_LEAF (status, strdup ("admin-status"),
                  g_strdup_printf ("%d", rtnl_link_get_flags (link) & IFF_UP ?
                                   INTERFACE_INTERFACES_STATUS_ADMIN_STATUS_ADMIN_UP :
                                   INTERFACE_INTERFACES_STATUS_ADMIN_STATUS_ADMIN_DOWN));
    APTERYX_LEAF (status, strdup ("oper-status"),
                  g_strdup_printf ("%d", rtnl_link_get_operstate (link)));
    APTERYX_LEAF (status, strdup ("flags"),
                  g_strdup_printf ("%d", rtnl_link_get_flags (link)));
    nl_addr2str (rtnl_link_get_addr (link), phys_address, sizeof (phys_address));
    APTERYX_LEAF (status, strdup ("phys-address"),
                  strdup (phys_address));
    APTERYX_LEAF (status, strdup ("promisc"),
                  g_strdup_printf ("%d", rtnl_link_get_promiscuity (link) ?
                                   INTERFACE_INTERFACES_STATUS_PROMISC_PROMISC_ON :
                                   INTERFACE_INTERFACES_STATUS_PROMISC_PROMISC_OFF));
    if (rtnl_link_get_qdisc (link))
        APTERYX_LEAF (status, strdup ("qdisc"),
                  strdup (rtnl_link_get_qdisc (link)));
    if (rtnl_link_get_mtu (link))
        APTERYX_LEAF (status, strdup ("mtu"),
                  g_strdup_printf ("%d", rtnl_link_get_mtu (link)));
    else
        APTERYX_LEAF (status, strdup ("mtu"),
                   g_strdup_printf ("%d", INTERFACE_INTERFACES_STATUS_MTU_DEFAULT));
    APTERYX_LEAF (status, strdup ("speed"),
            g_strdup_printf ("%d", if_speed_get (rtnl_link_get_name (link))));
    APTERYX_LEAF (status, strdup ("duplex"),
            g_strdup_printf ("%d", if_duplex_get (rtnl_link_get_name (link))));
    if (rtnl_link_get_arptype (link))
        APTERYX_LEAF (status, strdup ("arptype"),
                  g_strdup_printf ("%d", rtnl_link_get_arptype (link)));
    else
        APTERYX_LEAF (status, strdup ("arptype"),
                  g_strdup_printf ("%d", INTERFACE_INTERFACES_STATUS_ARPTYPE_DEFAULT));
    if (rtnl_link_get_num_rx_queues (link))
        APTERYX_LEAF (status, strdup ("rxq"),
                  g_strdup_printf ("%d", rtnl_link_get_num_rx_queues (link)));
    else
        APTERYX_LEAF (status, strdup ("rxq"),
                  g_strdup_printf ("%d", INTERFACE_INTERFACES_STATUS_RXQ_DEFAULT));
    if (rtnl_link_get_txqlen (link))
        APTERYX_LEAF (status, strdup ("txqlen"),
                  g_strdup_printf ("%d", rtnl_link_get_txqlen (link)));
    else
        APTERYX_LEAF (status, strdup ("txqlen"),
                  g_strdup_printf ("%d", INTERFACE_INTERFACES_STATUS_TXQLEN_DEFAULT));
    if (rtnl_link_get_num_tx_queues (link))
        APTERYX_LEAF (status, strdup ("txq"),
                  g_strdup_printf ("%d", rtnl_link_get_num_tx_queues (link)));
    else
        APTERYX_LEAF (status, strdup ("txq"),
                  g_strdup_printf ("%d", INTERFACE_INTERFACES_STATUS_TXQ_DEFAULT));

    return root;
}

/**
 * Netlink callback to capture interface state change
 * @param action NL_ACT_NEW, NL_ACT_DEL, NL_ACT_CHANGE
 * @param old_obj v2 callbacks provide a before link object
 * @param new_obj v1/v2 callbacks
 */
static void
nl_if_cb (int action, struct nl_object *old_obj, struct nl_object *new_obj)
{
    struct rtnl_link *link;
    char *path;

    /* Check action */
    if (action != NL_ACT_NEW && action != NL_ACT_DEL && action != NL_ACT_CHANGE)
    {
        ERROR ("IFSTATUS: invalid interface cb action:%d\n", action);
        return;
    }

    //TODO delta between old and new for new version of libnl
    if (old_obj && !new_obj)
        new_obj = old_obj;

    /* Check link object */
    if (!new_obj)
    {
        ERROR ("IFSTATUS: missing link object\n");
        return;
    }
    link = (struct rtnl_link *) new_obj;

    /* Debug */
    VERBOSE ("IFSTATUS: %s interface\n", action == NL_ACT_NEW ? "NEW" :
             (action == NL_ACT_DEL ? "DEL" : "CHG"));
    if (kermond_verbose)
        nl_object_dump (new_obj, &netlink_dp);

    /* Process action */
    if (action == NL_ACT_DEL)
    {
        /* Remove the if-alias */
        path = g_strdup_printf (INTERFACE_IF_ALIAS "/%d",
                                rtnl_link_get_ifindex (link));
        apteryx_set (path, NULL);
        free (path);

        /* Generate path to interface status information */
        path =
            g_strdup_printf (INTERFACE_INTERFACES_PATH "/%s/" INTERFACE_INTERFACES_STATUS_PATH,
                             rtnl_link_get_name (link));
        apteryx_prune (path);
        free (path);
    }
    else
    {
        /* Add/Update Apteryx */
        GNode *tree = link_to_apteryx (link);
        if (tree)
        {
            apteryx_set_tree (tree);
            apteryx_free_tree (tree);
        }
    }
}

/**
 * Remove any state we have stored in Apteryx
 */
static void
ifstatus_cleanup (void)
{
    /* Remove status for each interface */
    GList *iflist = apteryx_search (INTERFACE_INTERFACES_PATH "/");
    for (GList * iter = iflist; iter; iter = iter->next)
    {
        /* Prune the status tree */
        char *path = g_strdup_printf ("%s/" INTERFACE_INTERFACES_STATUS_PATH,
                                      (char *) iter->data);
        apteryx_prune (path);
        free (path);
    }
    g_list_free_full (iflist, free);

    /* Throw away the if-alias table */
    apteryx_prune (INTERFACE_IF_ALIAS);
}

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
