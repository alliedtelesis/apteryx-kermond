/**
 * @file module.c
 * Module abstraction
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

/* Modules */
extern struct module_table __start_module_table;
extern struct module_table __stop_module_table;

bool
modules_enable (const char *modules)
{
    gchar **list = g_strsplit (modules, ",", -1);
    gchar **plist = list;
    module_table *mt;

    /* Disable all modules */
    for (mt = &__start_module_table; mt != &__stop_module_table; mt++)
    {
        mt->enabled = false;
    }

    /* Enable required modules */
    while (*plist)
    {
        for (mt = &__start_module_table; mt != &__stop_module_table; mt++)
        {
            if (strcmp (*plist, mt->name) == 0)
            {
                mt->enabled = true;
                break;
            }
        }
        if (mt == &__stop_module_table)
        {
            ERROR ("MODULE: no such module \"%s\"\n", *plist);
            modules_dump ();
            g_strfreev (list);
            return false;
        }
        plist++;
    }
    g_strfreev (list);
    return true;
}

void
modules_dump ()
{
    module_table *mt;

    printf ("Modules: ");
    for (mt = &__start_module_table; mt != &__stop_module_table; mt++)
    {
        printf ("%s ", mt->name);
    }
    printf ("\n");
}

bool
modules_init (void)
{
    module_table *mt;

    for (mt = &__start_module_table; mt != &__stop_module_table; mt++)
    {
        if (!mt->enabled || !mt->init)
            continue;
        VERBOSE ("MODULE: Initialising %s\n", mt->name);
        if (!(*mt->init) ())
        {
            ERROR ("MODULE: Failed to initialise \"%s\"\n", mt->name);
            return false;
        }
    }
    return true;
}

bool
modules_start (void)
{
    module_table *mt;

    for (mt = &__start_module_table; mt != &__stop_module_table; mt++)
    {
        if (!mt->enabled || !mt->start)
            continue;
        VERBOSE ("MODULE: Starting %s\n", mt->name);
        if (!(*mt->start) ())
        {
            ERROR ("MODULE: Failed to start \"%s\"\n", mt->name);
            return false;
        }
    }
    return true;
}

void
modules_exit (void)
{
    module_table *mt;

    for (mt = &__stop_module_table - 1; mt >= &__start_module_table; mt--)
    {
        if (!mt->enabled || !mt->exit)
            continue;
        VERBOSE ("MODULE: Stopping %s\n", mt->name);
        (*mt->exit) ();
    }
    return;
}
