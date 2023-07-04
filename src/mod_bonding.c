/**
 * @file bonding.c
 * Manage bonded interfaces
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
#include "at-bonding.h"

/**
 * Process apteryx watch callback for settings
 * @param root data that has been updated
 * @return true if the callback was expected, false otherwise
 */
static bool
bonding_settings_cb (GNode *root)
{
    DEBUG ("BONDING: Update\n");

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
bonding_start ()
{
    DEBUG ("BONDING: Starting\n");

    /* Setup Apteryx watchers */
    apteryx_watch_tree (BONDING_BONDS_PATH "/*/" BONDING_BONDS_SETTINGS_PATH "/*", bonding_settings_cb);

    /* Load existing configuration */
    GNode *tree = apteryx_get_tree (BONDING_BONDS_PATH "/*/" BONDING_BONDS_SETTINGS_PATH);
    if (tree)
        bonding_settings_cb (tree);

    return true;
}

/**
 * Module shutdown
 */
static void
bonding_exit ()
{
    DEBUG ("BONDING: Exiting\n");

    /* Detach Apteryx watchers */
    apteryx_unwatch_tree (BONDING_BONDS_PATH "/*/" BONDING_BONDS_SETTINGS_PATH "/*", bonding_settings_cb);
}

MODULE_CREATE ("bonding", NULL, bonding_start, bonding_exit);
