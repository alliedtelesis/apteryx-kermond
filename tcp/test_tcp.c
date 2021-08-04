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

#include "test.h"

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
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_tcp_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("TCP: Unexpected path: (null)\n");
}

void test_tcp_invalid_path ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_tcp_settings (IP_TCP_PATH, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("TCP: Unexpected path: /ip/tcp\n");
}

void test_tcp_invalid_parameter ()
{
    NP_TEST_START
    setup_test (NULL, NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("TCP: Unexpected setting \"speed\"\n");
}

void test_tcp_synack_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_tcp_synack_value_0 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=0", NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_tcp_synack_value_255 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=255", NULL);
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "255"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_tcp_synack_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", "Invalid synack-retries");
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("TCP: Invalid synack-retries value (dog) using default (5)\n");
}

void test_tcp_synack_value_negative ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", "Invalid synack-retries");
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "-1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("TCP: Invalid synack-retries value (-1) using default (5)\n");
}

void test_tcp_synack_value_too_high ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.tcp_synack_retries=5", "Invalid synack-retries");
    NP_ASSERT_TRUE (watch_tcp_settings (IP_TCP_SYNACK_RETRIES, "256"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("TCP: Invalid synack-retries value (256) using default (5)\n");
}
