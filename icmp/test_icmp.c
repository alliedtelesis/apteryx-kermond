/**
 * @file test_icmp.c
 * Unit tests for ICMP configuration
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
#include "icmp.c"

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

void test_icmp4_path_null ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv4_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_icmp4_invalid_path ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv4_settings (IP_V4_ICMP_PATH, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_icmp4_invalid_parameter ()
{
    setup_test (NULL, NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_dest_unreach_value_null ()
{
    setup_test ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_dest_unreach_enable ()
{
    setup_test ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_dest_unreach_disable ()
{
    setup_test ("iptables -C ICMP -p icmp --icmp-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_dest_unreach_value_invalid ()
{
    setup_test ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP", "Invalid send-destination-unreachable");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_error_ratelimit_value_null ()
{
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_ERROR_RATELIMIT_PATH, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_error_ratelimit_value_0 ()
{
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=0", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_ERROR_RATELIMIT_PATH, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_error_ratelimit_value_2147483647 ()
{
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=2147483647", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_ERROR_RATELIMIT_PATH, "2147483647"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_error_ratelimit_value_invalid ()
{
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_ERROR_RATELIMIT_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_error_ratelimit_value_negative ()
{
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_ERROR_RATELIMIT_PATH, "-1"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv4_error_ratelimit_value_too_high ()
{
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_V4_ICMP_ERROR_RATELIMIT_PATH, "2147483648"));
    NP_ASSERT_NULL (cmds);
}

void test_icmp6_path_null ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv6_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_icmp6_invalid_path ()
{
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv6_settings (IP_V6_ICMP_PATH, "5"));
    NP_ASSERT_NULL (cmds);
}

void test_icmp6_invalid_parameter ()
{
    setup_test (NULL, NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_dest_unreach_value_null ()
{
    setup_test ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_dest_unreach_enable ()
{
    setup_test ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, "1"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_dest_unreach_disable ()
{
    setup_test ("ip6tables -C ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_dest_unreach_value_invalid ()
{
    setup_test ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", "Invalid send-destination-unreachable");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_SEND_DESTINATION_UNREACHABLE_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_error_ratelimit_value_null ()
{
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_ERROR_RATELIMIT_PATH, NULL));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_error_ratelimit_value_0 ()
{
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=0", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_ERROR_RATELIMIT_PATH, "0"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_error_ratelimit_value_2147483647 ()
{
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=2147483647", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_ERROR_RATELIMIT_PATH, "2147483647"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_error_ratelimit_value_invalid ()
{
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_ERROR_RATELIMIT_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_error_ratelimit_value_negative ()
{
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_ERROR_RATELIMIT_PATH, "-1"));
    NP_ASSERT_NULL (cmds);
}

void test_icmpv6_error_ratelimit_value_too_high ()
{
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_V6_ICMP_ERROR_RATELIMIT_PATH, "2147483648"));
    NP_ASSERT_NULL (cmds);
}
