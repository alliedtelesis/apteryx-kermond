/**
 * @file dynamic.c
 * Manage entities with dynamic IP addresses
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
#include "at-entity.h"

typedef enum
{
    dynamic_source_interface,
} dynamic_type;

struct dynamic_entity_info
{
    char *entity;
    dynamic_type type;
    int family;
    char *interface;
    bool deleted;
};

/* Fallback if we have no link cache */
extern unsigned int if_nametoindex (const char *ifname);
extern char *if_indextoname (unsigned int ifindex, char *ifname);

/* Required caches */
static struct nl_cache *addr_cache = NULL;
static struct nl_cache *link_cache = NULL;

/* Dynamic entity lists */
static GList *dynamic_entities = NULL;
static GMutex list_lock = { };

static char*
entity_path (char *zone, char *network, char *host)
{
    if (zone && network && host)
        return g_strdup_printf (ENTITIES_PATH "/%s/" ENTITIES_CHILDREN_PATH "/%s/"
                        ENTITIES_CHILDREN_CHILDREN_PATH "/%s",
                        zone, network, host);
    else if (zone && network)
        return g_strdup_printf (ENTITIES_PATH "/%s/" ENTITIES_CHILDREN_PATH "/%s/",
                zone, network);
    else if (zone)
        return g_strdup_printf (ENTITIES_PATH "/%s/", zone);
    return NULL;
}

static void
dynamic_enitity_add_del_address (struct nl_object *obj, void *arg)
{
    struct rtnl_addr *ra = (struct rtnl_addr *) obj;
    struct nl_addr *addr = rtnl_addr_get_local (ra);
    struct dynamic_entity_info *info = (struct dynamic_entity_info *) arg;
    int prefix_len = 0;
    char addr_str[128];

    /* Parse IP address */
    nl_addr2str (addr, addr_str, sizeof (addr_str));
    prefix_len = nl_addr_get_prefixlen (addr);

    VERBOSE ("ENTITY: %s %s %s %s", info->deleted ? "removing" : "adding", addr_str,
            info->deleted ? "from" : "to", info->entity);

    /* Update Apteryx */
    char *path = g_strdup_printf ("%s/subnets/dynamic_%s_%d",
            info->entity, addr_str, prefix_len);
    char *value = info->deleted ? NULL : g_strdup_printf ("%s/%d", addr_str, prefix_len);
    apteryx_set (path, value);
    free (path);
    free (value);
}

static void
dynamic_enitity_add_del_addresses (struct dynamic_entity_info *info)
{
    struct rtnl_addr *addr;
    int ifindex = 0;

    /* Check we have the address cache */
    if (!addr_cache)
    {
        ERROR ("ENTITY: No address cache!\n");
        return;
    }

    /* Find ifindex for interface */
    if (link_cache)
        ifindex = rtnl_link_name2i (link_cache, info->interface);
    else
        ifindex = if_nametoindex (info->interface);
    if (ifindex == 0)
    {
        DEBUG ("ENTITY: No ifindex for \"%s\"\n", info->interface);
        return;
    }
    addr = rtnl_addr_alloc ();
    rtnl_addr_set_family (addr, info->family);
    rtnl_addr_set_ifindex (addr, ifindex);
    nl_cache_foreach_filter (addr_cache, OBJ_CAST(addr),
            dynamic_enitity_add_del_address, info);
    rtnl_addr_put (addr);
}

static bool
dynamic_enitity_interface_change (int family,
        char *zone, char *network, char *host,
        const char *ifname, bool add)
{
    char *entity = entity_path (zone, network, host);
    if (add)
    {
        struct dynamic_entity_info *info = calloc (1, sizeof (*info));
        info->entity = entity;
        info->family = family;
        info->type = dynamic_source_interface;
        info->interface = strdup (ifname);
        info->deleted = false;
        g_mutex_lock (&list_lock);
        dynamic_entities = g_list_append (dynamic_entities, info);
        g_mutex_unlock (&list_lock);
        dynamic_enitity_add_del_addresses (info);
    }
    else
    {
        GList *iter = NULL, *next = NULL;
        g_mutex_lock (&list_lock);
        for (iter = g_list_first (dynamic_entities); iter; iter = next)
        {
            next = g_list_next (iter);
            struct dynamic_entity_info *info = iter->data;
            if (info->family == family &&
                info->type == dynamic_source_interface &&
                strcmp (info->interface, ifname) == 0 &&
                strcmp (info->entity, entity) == 0)
            {
                dynamic_entities = g_list_remove_link (dynamic_entities, iter);
                info->deleted = true;
                dynamic_enitity_add_del_addresses (info);
                free (info->entity);
                free (info->interface);
                free (info);
                g_list_free (iter);
            }
        }
        g_mutex_unlock (&list_lock);
        free (entity);
    }

    return true;
}

static void
nl_addr_cb (int action, struct nl_object *old_obj, struct nl_object *new_obj)
{
    struct rtnl_addr *ra;
    struct nl_addr *addr;
    int family;
    char ifname[IFNAMSIZ] = "";

    /* Check action */
    if (action != NL_ACT_NEW && action != NL_ACT_DEL)
    {
        ERROR ("ENTITY: invalid address cb action:%d\n", action);
        return;
    }

    //TODO delta between old and new for new version of libnl
    if (old_obj && !new_obj)
        new_obj = old_obj;

    /* Check address object */
    if (!new_obj)
    {
        ERROR ("ENTITY: missing address object\n");
        return;
    }

    /* Debug */
    VERBOSE ("ENTITY: %s address\n", action == NL_ACT_NEW ? "NEW" : "DEL");
    if (kermond_verbose)
        nl_object_dump (new_obj, &netlink_dp);

    /* Parse address and ifname */
    ra = (struct rtnl_addr *) new_obj;
    family = rtnl_addr_get_family (ra);
    addr = rtnl_addr_get_local (ra);
    if (link_cache)
        rtnl_link_i2name (link_cache, rtnl_addr_get_ifindex (ra), ifname, sizeof (ifname));
    else
        if_indextoname (rtnl_addr_get_ifindex (ra), ifname);
    if ((family != AF_INET && family != AF_INET6) ||
        !addr || rtnl_addr_get_ifindex (ra) == 0 || strlen (ifname) == 0)
    {
        ERROR ("ENTITY: invalid address object\n");
        return;
    }

    /* Find a dynamic entity */
    GList *iter = NULL;
    g_mutex_lock (&list_lock);
    for (iter = g_list_first (dynamic_entities); iter; iter = g_list_next (iter))
    {
        struct dynamic_entity_info *info = iter->data;
        if (info->type != dynamic_source_interface ||
            family != info->family || strcmp (ifname, info->interface) != 0)
        {
            continue;
        }

        char addr_str[INET6_ADDRSTRLEN + 1] = "";
        if (nl_addr2str (addr, addr_str, sizeof (addr_str)) == NULL)
        {
            ERROR ("ENTITY: failed to parse address\n");
            continue;
        }

        /* Update Apteryx */
        char *path = g_strdup_printf ("%s/subnets/dynamic_%s_%d",
                info->entity, addr_str, nl_addr_get_prefixlen (addr));
        char *value = action == NL_ACT_DEL ? NULL :
                g_strdup_printf ("%s/%d", addr_str, nl_addr_get_prefixlen (addr));
        apteryx_set (path, value);
        free (path);
        free (value);
    }
    g_mutex_unlock (&list_lock);
}

/**
 * Process apteryx watch callback for Entity settings
 * @param path path to variable that has changed
 * @param value new value (NULL means default)
 * @return true if the callback was expected, false otherwise
 */
static bool
watch_entities (const char *path, const char *value)
{
    char zone[64];
    char network[64];
    char host[64];
    int family;
    char ifname[64];

    /* Check dynamic host entities */
    if (path && sscanf (path, ENTITIES_PATH "/%64[^/]/" ENTITIES_CHILDREN_PATH "/%64[^/]/"
            ENTITIES_CHILDREN_CHILDREN_PATH "/%64[^/]/" ENTITIES_CHILDREN_CHILDREN_DYNAMIC_PATH
            "/ipv%d/interfaces/%64s",
            zone, network, host, &family, ifname) == 5)
    {
        VERBOSE ("ENTITY: Dynamic ipv%d host: \"%s.%s.%s\" interface \"%s\"\n",
                family, zone, network, host, ifname);
        if (value && strcmp (ifname, value) != 0)
        {
            ERROR ("ENTITY: Inconsistent interface: %s != %s\n", value, ifname);
            return false;
        }

        /* Add/remove the dynamic entity */
        dynamic_enitity_interface_change (family == 4 ? AF_INET : AF_INET6,
                zone, network, host, ifname, value != NULL);

        return true;
    }

    ERROR ("ENTITY: Unexpected path: %s\n", path);
    return false;
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
entity_start ()
{
    DEBUG ("ENTITY: Starting\n");

    /* Create the addr cache and register for callbacks */
    netlink_register ("route/addr", nl_addr_cb);
    addr_cache = nl_cache_mngt_require_safe ("route/addr");
    link_cache = nl_cache_mngt_require_safe ("route/link");

    /* Watch for changes in configuration */
    apteryx_watch (ENTITIES_PATH "/*", watch_entities);

    /* Load existing configuration */
    apteryx_rewatch_tree (ENTITIES_PATH, watch_entities);

    return true;
}

/**
 * Module shutdown
 */
static void
entity_exit ()
{
    DEBUG ("ENTITY: Exiting\n");

    /* Remove watch from Apteryx */
    apteryx_unwatch (ENTITIES_PATH "/*", watch_entities);

    /* Detach our callback and unref the addr cache */
    if (link_cache)
        nl_cache_put (link_cache);
    if (addr_cache)
        nl_cache_put (addr_cache);
    netlink_unregister ("route/addr", nl_addr_cb);
}

MODULE_CREATE ("entity", NULL, entity_start, entity_exit);
