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

void test_icmp4_path_null ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv4_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Unexpected path: (null)\n");
}

void test_icmp4_invalid_path ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv4_settings (IP_ICMP_IPV4_PATH, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Unexpected path: /ip/icmp/ipv4\n");
}

void test_icmp4_invalid_parameter ()
{
    NP_TEST_START
    setup_test (NULL, NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Unexpected setting \"speed\"\n");
}

void test_icmpv4_dest_unreach_value_null ()
{
    NP_TEST_START
    setup_test ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_SEND_DESTINATION_UNREACHABLE_PATH, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv4_dest_unreach_enable ()
{
    NP_TEST_START
    setup_test ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_SEND_DESTINATION_UNREACHABLE_PATH, "1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv4_dest_unreach_disable ()
{
    NP_TEST_START
    setup_test ("iptables -C ICMP -p icmp --icmp-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_SEND_DESTINATION_UNREACHABLE_PATH, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv4_dest_unreach_value_invalid ()
{
    NP_TEST_START
    setup_test ("iptables -D ICMP -p icmp --icmp-type destination-unreachable -j DROP",
            "Invalid /ip/icmp/ipv4/send-destination-unreachable value");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_SEND_DESTINATION_UNREACHABLE_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/icmp/ipv4/send-destination-unreachable value (dog) using default (1)\n");
}

void test_icmpv4_error_ratelimit_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_ERROR_RATELIMIT, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv4_error_ratelimit_value_0 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=0", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_ERROR_RATELIMIT, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv4_error_ratelimit_value_2147483647 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=2147483647", NULL);
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_ERROR_RATELIMIT, "2147483647"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv4_error_ratelimit_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_ERROR_RATELIMIT, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Invalid ratelimit value (dog) using default (1000)\n");
}

void test_icmpv4_error_ratelimit_value_negative ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_ERROR_RATELIMIT, "-1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Invalid ratelimit value (-1) using default (1000)\n");
}

void test_icmpv4_error_ratelimit_value_too_high ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv4.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv4_settings (IP_ICMP_IPV4_ERROR_RATELIMIT, "2147483648"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Invalid ratelimit value (2147483648) using default (1000)\n");
}

void test_icmp6_path_null ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv6_settings (NULL, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Unexpected path: (null)\n");
}

void test_icmp6_invalid_path ()
{
    NP_TEST_START
    setup_test (NULL, "Unexpected path");
    NP_ASSERT_FALSE (watch_icmpv6_settings (IP_ICMP_IPV6_PATH, "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Unexpected path: /ip/icmp/ipv6\n");
}

void test_icmp6_invalid_parameter ()
{
    NP_TEST_START
    setup_test (NULL, NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_PATH "/speed", "5"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Unexpected setting \"speed\"\n");
}

void test_icmpv6_dest_unreach_value_null ()
{
    NP_TEST_START
    setup_test ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_SEND_DESTINATION_UNREACHABLE_PATH, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv6_dest_unreach_enable ()
{
    NP_TEST_START
    setup_test ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_SEND_DESTINATION_UNREACHABLE_PATH, "1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv6_dest_unreach_disable ()
{
    NP_TEST_START
    setup_test ("ip6tables -C ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_SEND_DESTINATION_UNREACHABLE_PATH, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv6_dest_unreach_value_invalid ()
{
    NP_TEST_START
    setup_test ("ip6tables -D ICMP -p icmpv6 --icmpv6-type destination-unreachable -j DROP",
            "Invalid /ip/icmp/ipv6/send-destination-unreachable value");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_SEND_DESTINATION_UNREACHABLE_PATH, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("Invalid /ip/icmp/ipv6/send-destination-unreachable value (dog) using default (1)\n");
}

void test_icmpv6_error_ratelimit_value_null ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_ERROR_RATELIMIT, NULL));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv6_error_ratelimit_value_0 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=0", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_ERROR_RATELIMIT, "0"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv6_error_ratelimit_value_2147483647 ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=2147483647", NULL);
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_ERROR_RATELIMIT, "2147483647"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("");
}

void test_icmpv6_error_ratelimit_value_invalid ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_ERROR_RATELIMIT, "dog"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Invalid ratelimit value (dog) using default (1000)\n");
}

void test_icmpv6_error_ratelimit_value_negative ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_ERROR_RATELIMIT, "-1"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Invalid ratelimit value (-1) using default (1000)\n");
}

void test_icmpv6_error_ratelimit_value_too_high ()
{
    NP_TEST_START
    setup_test ("sysctl -w net.ipv6.icmp_ratelimit=1000", "Invalid ratelimit");
    NP_ASSERT_TRUE (watch_icmpv6_settings (IP_ICMP_IPV6_ERROR_RATELIMIT, "2147483648"));
    NP_ASSERT_NULL (cmds);
    NP_TEST_END ("ICMP: Invalid ratelimit value (2147483648) using default (1000)\n");
}
