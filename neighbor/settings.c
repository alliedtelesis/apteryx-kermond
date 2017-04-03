/**
 * @file settings.c
 * Manage kernel neighbor settings
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
#include "neighbor.h"

static bool
watch_ipv4_settings (const char *path, const char *value)
{
    char ifname[64];
    char parameter[64];
    char *cmd = NULL;

    DEBUG ("NEIGHBOR: %s = %s\n", path, value);

    /* Opportunistic Neighbor Discovery */
    if (path && strcmp (path, NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_PATH) == 0)
    {
        int mode = NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_DEFAULT;
        if ((value && sscanf (value, "%d", &mode) != 1) ||
            (mode != NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_ENABLED &&
             mode != NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_DISABLED))
        {
            mode = NEIGHBOR_IPV4_SETTINGS_OPPORTUNISTIC_ND_DEFAULT;
            ERROR ("NEIGHBOR: Invalid opportunistic-nd value (%s) using default (%d)\n",
                   value, mode);
        }
        cmd = g_strdup_printf ("sysctl -w net.ipv4.aggressive_nd=%d", mode);
    }
    /* Interface specific */
    else if (path && sscanf (path, NEIGHBOR_IPV4_SETTINGS_INTERFACES_PATH "/%64[^/]/%64s",
                     ifname, parameter) == 2)
    {
        /* Aging Timeout */
        if (strcmp (parameter, NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT) == 0)
        {
            int timeout = NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT_DEFAULT;
            if ((value && sscanf (value, "%d", &timeout) != 1) ||
                timeout < 0 || timeout > 432000)
            {
                timeout = NEIGHBOR_IPV4_SETTINGS_INTERFACES_AGING_TIMEOUT_DEFAULT;
                ERROR ("NEIGHBOR: Invalid aging-timeout value (%s) using default (%d)\n",
                       value, timeout);
            }
            cmd = g_strdup_printf ("sysctl -w net.ipv4.neigh.%s.base_reachable_time_ms=%d",
                                   ifname, timeout * 1000);
        }
        /* MAC Disparity */
        else if (strcmp (parameter, NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY) == 0)
        {
            int mode = NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY_DEFAULT;
            if ((value && sscanf (value, "%d", &mode) != 1) ||
                (mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY_ENABLED &&
                 mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY_DISABLED))
            {
                mode = NEIGHBOR_IPV4_SETTINGS_INTERFACES_MAC_DISPARITY_DEFAULT;
                ERROR ("NEIGHBOR: Invalid mac-disparity value (%s) using default (%d)\n",
                       value, mode);
            }
            cmd =
                g_strdup_printf ("sysctl -w net.ipv4.conf.%s.arp_mac_disparity=%d", ifname,
                                 mode);
        }
        /* Proxy Arp */
        else if (strcmp (parameter, NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP) == 0)
        {
            int mode = NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP_DEFAULT;
            if ((value && sscanf (value, "%u", &mode) != 1) ||
                (mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP_DISABLED &&
                 mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP_ENABLED &&
                 mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP_LOCAL &&
                 mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP_BOTH))
            {
                mode = NEIGHBOR_IPV4_SETTINGS_INTERFACES_PROXY_ARP_DEFAULT;
                ERROR ("NEIGHBOR: Invalid proxy-arp value (%s) using default (%d)\n", value,
                       mode);
            }
            cmd = g_strdup_printf ("sysctl -w net.ipv4.conf.%s.proxy_arp=%d", ifname, mode);
        }
        /* Opportunistic Neighbor Discovery */
        else if (strcmp (parameter, NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND) == 0)
        {
            int mode = NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND_DEFAULT;
            if ((value && sscanf (value, "%u", &mode) != 1) ||
                (mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND_DISABLED &&
                 mode != NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND_ENABLED))
            {
                mode = NEIGHBOR_IPV4_SETTINGS_INTERFACES_OPTIMISTIC_ND_DEFAULT;
                ERROR ("NEIGHBOR: Invalid opportunistic-nd value (%s) using default (%d)\n",
                       value, mode);
            }
            cmd =
                g_strdup_printf ("sysctl -w net.ipv4.neigh.%s.optimistic_nd=%d", ifname,
                                 mode);
        }
    }
    else
    {
        ERROR ("NEIGHBOR: Unexpected path: %s\n", path);
        return false;
    }

    /* Valid configuration change */
    if (cmd)
    {
        DEBUG ("NEIGHBOR: %s\n", cmd);

        if (system (cmd) != 0)
        {
            ERROR ("NEIGHBOR: Command failed (%s)\n", cmd);
        }
        free (cmd);
    }

    return true;
}

/**
 * Manage changes in IPv6 neighbor configuration
 * @param path path to configuration setting that has changed
 * @param value new value for the specified path
 * @return true if callback was expected
 */
static bool
watch_ipv6_settings (const char *path, const char *value)
{
    char *cmd = NULL;

    /* Opportunistic Neighbor Discovery */
    if (path && strcmp (path, NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_PATH) == 0)
    {
        int mode = NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_DEFAULT;
        if ((value && sscanf (value, "%d", &mode) != 1) ||
            (mode != NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_ENABLED &&
             mode != NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_DISABLED))
        {
            mode = NEIGHBOR_IPV6_SETTINGS_OPPORTUNISTIC_ND_DEFAULT;
            ERROR ("NEIGHBOR: Invalid opportunistic-nd value (%s) using default (%d)\n",
                   value, mode);
        }
        cmd = g_strdup_printf ("sysctl -w net.ipv6.icmp.aggressive_nd=%d", mode);
    }
    else
    {
        ERROR ("NEIGHBOR: Unexpected path: %s\n", path);
        return false;
    }

    /* Valid configuration change */
    if (cmd)
    {
        DEBUG ("NEIGHBOR: %s\n", cmd);

        if (system (cmd) != 0)
        {
            ERROR ("NEIGHBOR: Command failed (%s)\n", cmd);
        }
        free (cmd);
    }
    return true;
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
neighbor_start ()
{
    DEBUG ("NEIGHBOR: Starting\n");

    /* Setup Apteryx watchers */
    apteryx_watch (NEIGHBOR_IPV4_SETTINGS_PATH "/*", watch_ipv4_settings);
    apteryx_watch (NEIGHBOR_IPV6_SETTINGS_PATH "/*", watch_ipv6_settings);

    /* Load existing configuration */
    apteryx_rewatch_tree (NEIGHBOR_IPV4_SETTINGS_PATH, watch_ipv4_settings);
    apteryx_rewatch_tree (NEIGHBOR_IPV6_SETTINGS_PATH, watch_ipv6_settings);

    return true;
}

/**
 * Module shutdown
 */
static void
neighbor_exit ()
{
    DEBUG ("NEIGHBOR: Exiting\n");

    /* Remove Apteryx watchers */
    apteryx_unwatch (NEIGHBOR_IPV4_SETTINGS_PATH "/*", watch_ipv4_settings);
    apteryx_unwatch (NEIGHBOR_IPV6_SETTINGS_PATH "/*", watch_ipv6_settings);
}

MODULE_CREATE ("neighbor-settings", NULL, neighbor_start, neighbor_exit);
