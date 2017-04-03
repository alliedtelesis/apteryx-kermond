/**
 * @file apteryx.c
 * Helpers for apteryx
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

/**
 * Translate GNode to path/value to simulate watch events at startup
 * @param n node of the Apteryx configuration tree
 * @param data unused
 * @return false to continue traversing
 */
static gboolean
watch_cb (GNode * n, gpointer data)
{
    apteryx_watch_callback cb = (apteryx_watch_callback) data;
    char *path = apteryx_node_path (n->parent);
    cb (path, (char *) n->data);
    free (path);
    return false;
}

/**
 * Cause watch callbacks for existing nodes in a tree
 * @param path path to the tree of settings
 * @param cb function to call for each settings
 */
void
apteryx_rewatch_tree (char *path, apteryx_watch_callback cb)
{
    /* Load existing configuration */
    GNode *tree = apteryx_get_tree (path);
    if (tree)
    {
        /* Simulate watch callbacks for each parameter */
        g_node_traverse (tree, G_IN_ORDER, G_TRAVERSE_LEAVES, -1,
                watch_cb, (gpointer) cb);
        apteryx_free_tree (tree);
    }
}
