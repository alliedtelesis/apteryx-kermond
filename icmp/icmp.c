/**
 * @file icmp.c
 * Manage kernel ICMP configuration
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
#include "ip-icmp.h"

/**
 * Parse true/false
 * @param value string containing formatted true/false
 * @param defvalue result if there is no match or value is null
 * @return parsed boolean version of true/false
 */
static bool
parse_boolean (const char *path, const char *value, bool defvalue)
{
    if (value &&
       (strcasecmp (value, "true") == 0 ||
        strcasecmp (value, "yes") == 0 ||
        strcmp (value, "1") == 0))
    {
        return true;
    }
    else if (value &&
            (strcasecmp (value, "false") == 0 ||
             strcasecmp (value, "no") == 0 ||
             strcmp (value, "0") == 0))
    {
        return false;
    }
    else if (value)
    {
        ERROR ("ICMP: Invalid %s value (%s) using default (%d)\n",
                path, value, defvalue);
    }
    return defvalue;
}

/**
 * Process apteryx watch callback for ICMPv4 settings
 * @param path path to variable that has changed
 * @param value new value (NULL means default)
 * @return true if the callback was expected, false otherwise
 */
static bool
watch_icmpv4_settings (const char *path, const char *value)
{
    char parameter[64];
    char *cmd = NULL;
    int val;
    int rc = 0;

    /* Parse family, index and the parameter that has changed */
    if (!path || sscanf (path, IP_ICMP_IPV4_PATH "/%64s", parameter) != 1)
    {
        ERROR ("ICMP: Unexpected path: %s\n", path);
        return false;
    }

    /* send-destination-unreachable */
    if (strcmp (parameter, "send-destination-unreachable") == 0)
    {
        if (parse_boolean (path, value, true) == false)
        {
            /* Add the rule only if it does not exist in the table */
            rc = system
                ("iptables -C ICMP -p icmp --icmp-type destination-unreachable -j DROP");
            if (rc != 0)
            {
                rc = system
                    ("iptables -I ICMP -p icmp --icmp-type destination-unreachable -j DROP");
            }
        }
        else
        {
            rc = system
                ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP");
        }
    }
    /* error-ratelimit */
    else if (strcmp (parameter, "error-ratelimit") == 0)
    {
        /* Use default if an invalid value is set */
        if (!value || sscanf (value, "%d", &val) != 1 || val < 0 || val > 2147483647)
        {
            val = IP_ICMP_IPV4_ERROR_RATELIMIT_DEFAULT;
            if (value)
            {
                ERROR ("ICMP: Invalid ratelimit value (%s) using default (%d)\n", value, val);
            }
        }
        cmd = g_strdup_printf ("sysctl -w net.ipv4.icmp_ratelimit=%d", val);
    }
    else
    {
        DEBUG ("ICMP: Unexpected setting \"%s\"\n", parameter);
        return true;
    }

    /* Make a change if required */
    if (cmd)
    {
        VERBOSE ("ICMP: %s\n", cmd);
        rc = system (cmd);
        free (cmd);
    }

    return (rc == 0);
}

/**
 * Process apteryx watch callback for ICMPv6 settings
 * @param path path to variable that has changed
 * @param value new value (NULL means default)
 * @return true if the callback was expected, false otherwise
 */
static bool
watch_icmpv6_settings (const char *path, const char *value)
{
    char parameter[64];
    char *cmd = NULL;
    int val;
    int rc = 0;

    /* Parse family, index and the parameter that has changed */
    if (!path || sscanf (path, IP_ICMP_IPV6_PATH "/%64s", parameter) != 1)
    {
        ERROR ("ICMP: Unexpected path: %s\n", path);
        return false;
    }

    /* send-destination-unreachable */
    if (strcmp (parameter, "send-destination-unreachable") == 0)
    {
        if (parse_boolean (path, value, true) == false)
        {
            /* Add the rule only if it does not exist in the table */
            rc = system
                ("ip6tables -C ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP");
            if (rc != 0)
            {
                rc = system
                    ("ip6tables -I ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP");
            }
        }
        else
        {
            rc = system
                ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP");
        }
    }
    /* error-ratelimit */
    else if (strcmp (parameter, "error-ratelimit") == 0)
    {
        /* Use default if an invalid value is set */
        if (!value || sscanf (value, "%d", &val) != 1 || val < 0 || val > 2147483647)
        {
            val = IP_ICMP_IPV6_ERROR_RATELIMIT_DEFAULT;
            if (value)
            {
                ERROR ("ICMP: Invalid ratelimit value (%s) using default (%d)\n", value, val);
            }
        }
        cmd = g_strdup_printf ("sysctl -w net.ipv6.icmp_ratelimit=%d", val);
    }
    else
    {
        DEBUG ("ICMP: Unexpected setting \"%s\"\n", parameter);
        return true;
    }

    /* Make a change if required */
    if (cmd)
    {
        VERBOSE ("ICMP: %s\n", cmd);
        rc = system (cmd);
        free (cmd);
    }

    return (rc == 0);
}

/**
 * Module initialisation
 * @return true on success, false on failure
 */
static bool
icmp_init (void)
{
    int rc;

    DEBUG ("ICMP: Initialising\n");

    /* Add chain for ICMP rules */
    rc = system ("iptables -N ICMP");
    syslog (LOG_DEBUG, "iptables add chain return value: %d", rc);
    rc = system ("ip6tables -N ICMP");
    syslog (LOG_DEBUG, "ip6tables add chain return value: %d", rc);

    return true;
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
icmp_start ()
{
    DEBUG ("ICMP: Starting\n");

    /* Add Apteryx watches */
    apteryx_watch (IP_ICMP_IPV4_PATH "/*", watch_icmpv4_settings);
    apteryx_watch (IP_ICMP_IPV6_PATH "/*", watch_icmpv6_settings);

    /* Load existing configuration */
    apteryx_rewatch_tree (IP_ICMP_IPV4_PATH, watch_icmpv4_settings);
    apteryx_rewatch_tree (IP_ICMP_IPV6_PATH, watch_icmpv6_settings);

    return true;
}

/**
 * Module shutdown
 */
static void
icmp_exit ()
{
    DEBUG ("ICMP: Exiting\n");

    /* Remove Apteryx watches */
    apteryx_unwatch (IP_ICMP_IPV4_PATH "/*", watch_icmpv4_settings);
    apteryx_unwatch (IP_ICMP_IPV6_PATH "/*", watch_icmpv6_settings);
}

MODULE_CREATE ("icmp", icmp_init, icmp_start, icmp_exit);
