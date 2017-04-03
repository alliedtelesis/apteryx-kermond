/**
 * @file main.c
 * Entry point for apteryx-kermond
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

/* PID file */
#define APTERYX_KERMOND_PID "/var/run/apteryx-kermond.pid"

/* Mainloop handle */
GMainLoop *g_loop = NULL;

/* Debug */
bool kermond_debug = false;
bool kermond_verbose = false;

static gboolean
termination_handler (gpointer arg1)
{
    GMainLoop *loop = (GMainLoop *) arg1;
    g_main_loop_quit (loop);
    return false;
}

void
help (char *app_name)
{
    printf ("Usage: %s [-h] [-b] [-v] [-d] [-p <pidfile>]\n"
            "  -h   show this help\n"
            "  -b   background mode\n"
            "  -d   enable debug\n"
            "  -v   enable verbose debug\n"
            "  -m   comma separated list of modules to load (e.g. ifconfig,ifstatus)\n"
            "  -p   use <pidfile> (defaults to " APTERYX_KERMOND_PID ")\n", app_name);
    modules_dump ();
}

int
main (int argc, char *argv[])
{
    const char *pid_file = APTERYX_KERMOND_PID;
    int i = 0;
    bool background = false;
    FILE *fp = NULL;

    /* Parse options */
    while ((i = getopt (argc, argv, "hdvbm:p:")) != -1)
    {
        switch (i)
        {
        case 'd':
            kermond_debug = true;
            background = false;
            break;
        case 'v':
            kermond_debug = true;
            kermond_verbose = true;
            background = false;
            break;
        case 'b':
            background = true;
            break;
        case 'm':
            if (!modules_enable (optarg))
                return 0;
            break;
        case 'p':
            pid_file = optarg;
            break;
        case '?':
        case 'h':
        default:
            help (argv[0]);
            return 0;
        }
    }

    /* Daemonize */
    if (background && fork () != 0)
    {
        /* Parent */
        return 0;
    }

    /* Create GLib loop early */
    g_loop = g_main_loop_new (NULL, true);

    /* Initialise Apteryx client library */
    apteryx_init (kermond_verbose);

    /* Initialise Netlink helper */
    if (!netlink_init ())
        goto exit;

    /* Initialise modules */
    if (!modules_init ())
        goto exit;

    /* Start the modules */
    if (!modules_start ())
        goto exit;

    /* Create pid file */
    if (background)
    {
        fp = fopen (pid_file, "w");
        if (!fp)
        {
            ERROR ("Failed to create PID file %s\n", pid_file);
            goto exit;
        }
        fprintf (fp, "%d\n", getpid ());
        fclose (fp);
    }

    /* GLib main loop with graceful termination */
    g_unix_signal_add (SIGINT, termination_handler, g_loop);
    g_unix_signal_add (SIGTERM, termination_handler, g_loop);
    signal (SIGPIPE, SIG_IGN);
    g_main_loop_run (g_loop);

  exit:

    /* Shutdown modules */
    modules_exit ();

    /* Cleanup netlink helper */
    netlink_exit ();

    /* Cleanup client library */
    apteryx_shutdown ();

    /* GLib main loop is done */
    g_main_loop_unref (g_loop);

    /* Remove the pid file */
    if (background)
        unlink (pid_file);

    return 0;
}
