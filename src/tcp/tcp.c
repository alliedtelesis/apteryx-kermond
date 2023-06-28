/**
 * @file tcp.c
 * Manage kernel TCP configuration
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
#include "ip-tcp.h"

/**
 * Process apteryx watch callback for TCP settings
 * @param path path to variable that has changed
 * @param value new value (NULL means default)
 * @return true if the callback was expected, false otherwise
 */
static bool
watch_tcp_settings (const char *path, const char *value)
{
    char parameter[64];
    char *cmd = NULL;
    int rc = 0;

    /* Parse family, index and the parameter that has changed */
    if (!path || sscanf (path, IP_TCP_PATH "/%64s", parameter) != 1)
    {
        ERROR ("TCP: Unexpected path: %s\n", path);
        return false;
    }

    /* synack-retries */
    if (strcmp (parameter, "synack-retries") == 0)
    {
        int val;

        /* Use default if an invalid value is set */
        if (!value || sscanf (value, "%d", &val) != 1 || val < 0 || val > 255)
        {
            val = IP_TCP_SYNACK_RETRIES_DEFAULT;
            if (value)
            {
                ERROR ("TCP: Invalid synack-retries value (%s) using default (%d)\n",
                       value, val);
            }
        }
        cmd = g_strdup_printf ("sysctl -w net.ipv4.tcp_synack_retries=%d", val);
    }
    else
    {
        DEBUG ("TCP: Unexpected setting \"%s\"\n", parameter);
        return true;
    }

    /* Make a change if required */
    if (cmd)
    {
        VERBOSE ("TCP: %s\n", cmd);
        rc = system (cmd);
        free (cmd);
    }

    return (rc == 0);
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
tcp_start ()
{
    DEBUG ("TCP: Starting\n");

    /* Watch for changes in configuration */
    apteryx_watch (IP_TCP_PATH "/*", watch_tcp_settings);

    /* Load existing configuration */
    apteryx_rewatch_tree (IP_TCP_PATH, watch_tcp_settings);

    return true;
}

/**
 * Module shutdown
 */
static void
tcp_exit ()
{
    DEBUG ("TCP: Exiting\n");

    /* Remove watch from Apteryx */
    apteryx_unwatch (IP_TCP_PATH "/*", watch_tcp_settings);
}

MODULE_CREATE ("tcp", NULL, tcp_start, tcp_exit);
