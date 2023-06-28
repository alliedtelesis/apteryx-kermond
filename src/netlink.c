/**
 * @file netlink.c
 * Netlink interface to the kernel
 * - Watches the kernel for changes
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

/* Netlink debug paramters */
struct nl_dump_params netlink_dp = { };

typedef struct cache_descriptor
{
    char *kind;
    struct nl_cache *cache;
    GList *callbacks;
} cache_descriptor;
static GList *caches = NULL;

static pthread_t monitor_thread = -1;
static struct nl_cache_mngr *mngr = NULL;

static void *
netlink_monitor (void *p)
{
    int err;

    DEBUG ("NETLINK: Starting monitor\n");
    while (err >= 0 && g_main_loop_is_running (g_loop))
    {
        err = nl_cache_mngr_poll (mngr, 1000);
        if (err < 0 && err != -NLE_INTR)
        {
            FATAL ("Netlink: failed to poll cache mngr: %s\n", nl_geterror (err));
            break;
        }
    }
    DEBUG ("NETLINK: Monitor exiting\n");
    return NULL;
}

static void
startup_cb (struct nl_object *obj, void *p)
{
    netlink_callback cb = (netlink_callback) p;
    cb (NL_ACT_NEW, NULL, obj);
}

static void
#if LIBNL_VER_MAJ < 3 || (LIBNL_VER_MAJ >= 3 && LIBNL_VER_MIN < 3)
change_cb (struct nl_cache *cache, struct nl_object *new_obj, int action, void *p)
{
    struct nl_object *old_obj = NULL;
#else
change_cb (struct nl_cache *cache, struct nl_object *old_obj,
           struct nl_object *new_obj, uint64_t dummy, int action, void *p)
{
#endif
    cache_descriptor *desc = (cache_descriptor *) p;
    GList *iter;

    for (iter = g_list_first (desc->callbacks); iter; iter = g_list_next (iter))
    {
        netlink_callback cb = (netlink_callback) iter->data;
        cb (action, old_obj, new_obj);
    }
}

bool
netlink_register (char *kind, netlink_callback cb)
{
    cache_descriptor *desc = NULL;
    GList *iter;
    int err;

    /* Check netlink has been initialised */
    if (!mngr)
    {
        FATAL ("Netlink: Not initialised\n");
        return false;
    }

    /* Check if we already have this cache */
    for (iter = g_list_first (caches); iter; iter = g_list_next (iter))
    {
        desc = (cache_descriptor *) iter->data;
        if (strcmp (kind, desc->kind) == 0)
            break;
        desc = NULL;
    }

    /* Create a new entry if required */
    if (desc == NULL)
    {
        desc = calloc (1, sizeof (cache_descriptor));
        desc->kind = strdup (kind);
        caches = g_list_prepend (caches, desc);

        /* Allocate the cache */
        err = nl_cache_alloc_name (kind, &desc->cache);
        if (err < 0)
        {
            FATAL ("Netlink: Allocate caches failed: %s\n", nl_geterror (err));
            return false;
        }
        if (err == 0)
#if LIBNL_VER_MAJ < 3 || (LIBNL_VER_MAJ >= 3 && LIBNL_VER_MIN < 3)
            err = nl_cache_mngr_add_cache (mngr, desc->cache, change_cb, desc);
#else
            err = nl_cache_mngr_add_cache_v2 (mngr, desc->cache, change_cb, desc);
#endif
        if (err < 0)
        {
            FATAL ("Netlink: Watch cache change failed: %s\n", nl_geterror (err));
            return false;
        }
    }
    desc->callbacks = g_list_prepend (desc->callbacks, cb);

    /* Force callbacks for all items */
    nl_cache_foreach (desc->cache, startup_cb, cb);
    return true;
}

void
netlink_unregister (char *kind, netlink_callback cb)
{
    cache_descriptor *desc = NULL;
    GList *iter;

    /* Find the cache descriptor */
    for (iter = g_list_first (caches); iter; iter = g_list_next (iter))
    {
        desc = (cache_descriptor *) iter->data;
        if (strcmp (kind, desc->kind) == 0)
            break;
        desc = NULL;
    }
    if (desc == NULL)
        return;

    /* Remove our callback */
    desc->callbacks = g_list_remove (desc->callbacks, cb);

    /* Destroy if their are no other users */
    if (desc->callbacks == NULL)
    {
        caches = g_list_remove (caches, desc);
        free (desc->kind);
        free (desc);
    }
}

bool
netlink_init (void)
{
    DEBUG ("NETLINK: Initialising\n");

    /* Debug parameters */
    netlink_dp.dp_type = NL_DUMP_DETAILS;
    netlink_dp.dp_fd = stdout;

    /* Cache manager */
    nl_cache_mngr_alloc (NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);

    /* Create the monitoring thread */
    pthread_create (&monitor_thread, NULL, netlink_monitor, NULL);
    pthread_setname_np (monitor_thread, "netlink");
    pthread_setschedprio (monitor_thread, -10);

    return true;
}

void
netlink_exit ()
{
    /* Check we are actually initialised */
    if (!mngr)
        return;

    DEBUG ("NETLINK: Exiting\n");

    /* Remove the netlink monitoring thread */
    if (monitor_thread != -1)
        pthread_join (monitor_thread, NULL);

    /* Free the cache manager */
    nl_cache_mngr_free (mngr);
}
