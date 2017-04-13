/**
 * @file ifstatus.c
 * Monitor kernel interfaces
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

/**
 * Retrieve the interface speed from the kernel
 * @param name interface name
 * @return the interface speed
 */
static unsigned int
if_speed_get (char *name)
{
    uint32_t speed = 0;
    char *file_name = NULL;

    if (asprintf (&file_name, "/sys/class/net/%s/speed", name) > 0)
    {
        speed = procfs_read_uint32 (file_name);
        free (file_name);
    }
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

    if (asprintf (&file_name, "/sys/class/net/%s/duplex", name) > 0)
    {
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
    }
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
    ifalias = apteryx_path_to_node (root, INTERFACE_IF_ALIAS_PATH, NULL);
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
    status = APTERYX_NODE (node, strdup (INTERFACE_INTERFACES_STATUS));
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
        path = g_strdup_printf (INTERFACE_IF_ALIAS_PATH "/%d",
                                rtnl_link_get_ifindex (link));
        apteryx_set (path, NULL);
        free (path);

        /* Generate path to interface status information */
        path =
            g_strdup_printf (INTERFACE_INTERFACES_PATH "/%s/" INTERFACE_INTERFACES_STATUS,
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
        char *path = g_strdup_printf ("%s/" INTERFACE_INTERFACES_STATUS,
                                      (char *) iter->data);
        apteryx_prune (path);
        free (path);
    }
    g_list_free_full (iflist, free);

    /* Throw away the if-alias table */
    apteryx_prune (INTERFACE_IF_ALIAS_PATH);
}

/**
 * Module initialisation
 * @return true on success
 */
bool
ifstatus_init (void)
{
    DEBUG ("IFSTATUS: Initialising\n");

    /* Cleanup any existing status information */
    ifstatus_cleanup ();

    /* Create the link cache and register for callbacks */
    netlink_register ("route/link", nl_if_cb);

    return true;
}

/**
 * Module shutdown
 */
static void
ifstatus_exit ()
{
    DEBUG ("IFSTATUS: Exiting\n");

    /* Detach our callback and unref the link cache */
    netlink_unregister ("route/link", nl_if_cb);

    /* Cleanup any status information we created */
    ifstatus_cleanup ();
}

MODULE_CREATE ("ifstatus", ifstatus_init, NULL, ifstatus_exit);
