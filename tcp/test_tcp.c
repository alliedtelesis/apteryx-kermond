/**
 * @file test_tcp.c
 * Unit tests for TCP configuration
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
#include "tcp.c"

#include <np.h>

static GList *cmds = NULL;

static int
mock_system (const char *cmd)
{
    GList *found = g_list_find_custom (cmds, cmd, (GCompareFunc) strcmp);
    NP_ASSERT_NOT_NULL (found);
    if (found)
    {
        free (found->data);
        cmds = g_list_delete_link (cmds, found);
    }
    return 0;
}

static void
setup_test (char *cmd, char *ignore)
{
    if (cmd)
        cmds = g_list_append (cmds, strdup (cmd));
    np_mock (system, mock_system);
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_tcp_path_null ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_tcp_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_invalid_path ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_tcp_settings (IP_TCP_PATH, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_invalid_parameter ()
{
    setup_test (NULL, NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_synack_value_null ()
{
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_synack_value_0 ()
{
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=0", NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_synack_value_255 ()
{
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=255", NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "255"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_synack_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", "Invalid synack-retries");
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "dog"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_synack_value_negative ()
{
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", "Invalid synack-retries");
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "-1"));
    NP_ASSERT_NULL (cmds);
}

void test_tcp_synack_value_too_high ()
{
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", "Invalid synack-retries");
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "256"));
    NP_ASSERT_NULL (cmds);
}
