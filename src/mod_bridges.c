/**
 * @file bridges.c
 * Manage brigded interfaces
 *
 * Copyright 2023, Allied Telesis Labs New Zealand, Ltd
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
#include "at-bridges.h"

/**
 * Process apteryx watch callback for settings
 * @param root data that has been updated
 * @return true if the callback was expected, false otherwise
 */
static bool
bridges_settings_cb (GNode *root)
{
    DEBUG ("BRIDGES: Update\n");

    if (kermond_verbose) {
        apteryx_print_tree (root, stdout);
    }
    apteryx_free_tree (root);
    return true;
}

/**
 * Module startup
 * @return true on success, false otherwise
 */
static bool
bridges_start ()
{
    DEBUG ("BRIDGES: Starting\n");

    /* Setup Apteryx watchers */
    apteryx_watch_tree (BRIDGES_PATH "/*", bridges_settings_cb);

    /* Load existing configuration */
    GNode *tree = apteryx_get_tree (BRIDGES_PATH);
    if (tree)
        bridges_settings_cb (tree);

    return true;
}

/**
 * Module shutdown
 */
static void
bridges_exit ()
{
    DEBUG ("BRIDGES: Exiting\n");

    /* Detach Apteryx watchers */
    apteryx_unwatch_tree (BRIDGES_PATH "/*", bridges_settings_cb);
}

MODULE_CREATE ("bridges", NULL, bridges_start, bridges_exit);
