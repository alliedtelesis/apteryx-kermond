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
#include "ip-neighbor.h"

static bool
system_call (char *cmd)
{
    VERBOSE ("NEIGHBOR: %s\n", cmd);
    if (system (cmd) != 0)
    {
        ERROR ("NEIGHBOR: Command failed (%s)\n", cmd);
        return false;
    }
    free (cmd);
    return true;
}

static bool
watch_ipv4_settings (const char *path, const char *value)
{
    char ifname[64];
    char parameter[64];

    VERBOSE ("NEIGHBOR: %s = %s\n", path, value);

    /* Opportunistic Neighbor Discovery */
    if (path && strcmp (path, IP_NEIGHBOR_IPV4_OPPORTUNISTIC_ND_PATH) == 0)
    {
        int mode = apteryx_parse_boolean (path, value, false) ? 1 : 0;
        char *cmd = g_strdup_printf ("sysctl -w net.ipv4.aggressive_nd=%d", mode);
        return system_call (cmd);
    }
    /* Interface specific */
    else if (path && sscanf (path, IP_NEIGHBOR_IPV4_INTERFACES_PATH "/%64[^/]/%64s",
                     ifname, parameter) == 2)
    {
        /* Aging Timeout */
        if (strcmp (parameter, IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT) == 0)
        {
            int timeout = IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT_DEFAULT;
            if ((value && sscanf (value, "%d", &timeout) != 1) ||
                timeout < 0 || timeout > 432000)
            {
                timeout = IP_NEIGHBOR_IPV4_INTERFACES_AGING_TIMEOUT_DEFAULT;
                ERROR ("NEIGHBOR: Invalid aging-timeout value (%s) using default (%d)\n",
                       value, timeout);
            }
            char *cmd = g_strdup_printf ("sysctl -w net.ipv4.neigh.%s.base_reachable_time_ms=%d",
                                   ifname, timeout * 1000);
            return system_call (cmd);
        }
        /* MAC Disparity */
        else if (strcmp (parameter, IP_NEIGHBOR_IPV4_INTERFACES_MAC_DISPARITY) == 0)
        {
            int mode = apteryx_parse_boolean (path, value, false) ? 1 : 0;
            char *cmd = g_strdup_printf ("sysctl -w net.ipv4.conf.%s.arp_mac_disparity=%d",
                        ifname, mode);
            return system_call (cmd);
        }
        /* Proxy Arp */
        else if (strcmp (parameter, IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP) == 0)
        {
            int mode = 0;
            if (value && strcmp (value,
                    IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP_DISABLED) == 0)
            {
                mode = 0;
            }
            else if (value && strcmp (value,
                    IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP_ENABLED) == 0)
            {
                mode = 1;
            }
            else if (value && strcmp (value,
                    IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP_LOCAL) == 0)
            {
                mode = 2;
            }
            else if (value && strcmp (value,
                    IP_NEIGHBOR_IPV4_INTERFACES_PROXY_ARP_BOTH) == 0)
            {
                mode = 3;
            }
            else if (value)
            {
                ERROR ("Invalid %s value (%s) using default (%d)\n",
                                path, value, mode);
            }
            char *cmd = g_strdup_printf ("sysctl -w net.ipv4.conf.%s.proxy_arp=%d", ifname, mode);
            return system_call (cmd);
        }
        /* Opportunistic Neighbor Discovery */
        else if (strcmp (parameter, IP_NEIGHBOR_IPV4_INTERFACES_OPTIMISTIC_ND) == 0)
        {
            int mode = apteryx_parse_boolean (path, value, true) ? 1 : 0;
            char *cmd = g_strdup_printf ("sysctl -w net.ipv4.neigh.%s.optimistic_nd=%d",
                        ifname, mode);
            return system_call (cmd);
        }
    }
    ERROR ("NEIGHBOR: Unexpected path: %s\n", path);
    return false;
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
    /* Opportunistic Neighbor Discovery */
    if (path && strcmp (path, IP_NEIGHBOR_IPV6_OPPORTUNISTIC_ND_PATH) == 0)
    {
        int mode = apteryx_parse_boolean (path, value, false) ? 1 : 0;
        char *cmd = g_strdup_printf ("sysctl -w net.ipv6.icmp.aggressive_nd=%d", mode);
        return system_call (cmd);
    }
    ERROR ("NEIGHBOR: Unexpected path: %s\n", path);
    return false;
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
    apteryx_watch (IP_NEIGHBOR_IPV4_PATH "/*", watch_ipv4_settings);
    apteryx_watch (IP_NEIGHBOR_IPV6_PATH "/*", watch_ipv6_settings);

    /* Load existing configuration */
    apteryx_rewatch_tree (IP_NEIGHBOR_IPV4_PATH, watch_ipv4_settings);
    apteryx_rewatch_tree (IP_NEIGHBOR_IPV6_PATH, watch_ipv6_settings);

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
    apteryx_unwatch (IP_NEIGHBOR_IPV4_PATH "/*", watch_ipv4_settings);
    apteryx_unwatch (IP_NEIGHBOR_IPV6_PATH "/*", watch_ipv6_settings);
}

MODULE_CREATE ("neighbor-settings", NULL, neighbor_start, neighbor_exit);
