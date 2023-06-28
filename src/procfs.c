/**
 * @file procfs.c
 * Utilities for reading/writing to procfs
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

uint32_t
procfs_read_uint32 (const char *path)
{
    uint32_t value;
    FILE *fp = fopen (path, "r");
    if (!fp || fscanf (fp, "%u", &value) != 1)
    {
        value = 0;
    }
    if (fp)
    {
        fclose (fp);
    }
    return value;
}

char *
procfs_read_string (const char *path)
{
    static char buffer[512];
    FILE *fp = fopen (path, "r");
    if (!fp || fscanf (fp, "%512s", buffer) != 1)
    {
        return NULL;
    }
    if (fp)
    {
        fclose (fp);
    }
    return buffer;
}

void
procfs_write_uint32 (const char *path, uint32_t value)
{
    FILE *fp = NULL;
    fp = fopen (path, "w");
    if (fp)
    {
        fprintf (fp, "%u", value);
        fflush (fp);
        fclose (fp);
    }
}
