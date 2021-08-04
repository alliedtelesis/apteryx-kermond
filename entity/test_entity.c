/**
 * @file test_dynamic.c
 * Unit tests for dynamic entities
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
#include "entity.c"
#include "test.h"

// apteryx -s /entities/private/children/lan/children/server/dynamic/ipv4/interfaces/p5p1 p5p1
#define ZONE    ENTITIES_PATH "/private"
#define NETWORK ZONE "/" ENTITIES_CHILDREN "/lan"
#define HOST    NETWORK "/" ENTITIES_CHILDREN_CHILDREN "/server"
#define DYNv4   HOST "/" ENTITIES_CHILDREN_CHILDREN_DYNAMIC_IPV4_INTERFACES "/" IFNAME
#define DYNv6   HOST "/" ENTITIES_CHILDREN_CHILDREN_DYNAMIC_IPV6_INTERFACES "/" IFNAME

static struct nl_object *
make_addr (int family, char *addr_str)
{
    struct nl_addr *a = NULL;
    if (addr_str)
    {
        if (family == AF_INET)
            nl_addr_parse (addr_str, family, &a);
        else if (family == AF_INET6)
            nl_addr_parse (addr_str, family, &a);
        else
            return NULL;
    }
    struct rtnl_addr *addr = rtnl_addr_alloc ();
    rtnl_addr_set_family (addr, family);
    rtnl_addr_set_ifindex (addr, IFINDEX);
    if (a)
    {
        rtnl_addr_set_local (addr, a);
        nl_addr_put (a);
    }
    return (struct nl_object *) addr;
}

static void
setup_test (bool active, int family, char *ignore)
{
    np_mock (if_nametoindex, mock_if_nametoindex);
    np_mock (if_indextoname, mock_if_indextoname);
    np_mock (nl_cache_foreach_filter, mock_nl_cache_foreach_filter);
    np_mock (apteryx_set_full, mock_apteryx_set_full);
    addr_family = family;
    addr_cache = (struct nl_cache *) ~0;
    link_active = active;
    if (ignore)
        np_syslog_ignore (ignore);
}

void test_entity_path_null ()
{
    NP_TEST_START
    setup_test (false, AF_INET, "Unexpected path");
    NP_ASSERT_FALSE (watch_entities (NULL, "5"));
    NP_TEST_END ("ENTITY: Unexpected path: (null)\n");
}

void test_entity_invalid_path ()
{
    NP_TEST_START
    setup_test (false, AF_INET, "Unexpected path");
    NP_ASSERT_FALSE (watch_entities (ENTITIES_PATH, "5"));
    NP_TEST_END ("ENTITY: Unexpected path: /entities\n");
}

void test_entity_dynamic_ipv4_inconsistent_ifname ()
{
    NP_TEST_START
    setup_test (false, AF_INET, "Inconsistent interface");
    NP_ASSERT_FALSE (watch_entities (DYNv4, "eth1"));
    NP_TEST_END ("ENTITY: Inconsistent interface: eth1 != eth99\n");
}

void test_entity_new_dynamic_ipv4_no_interface ()
{
    NP_TEST_START
    setup_test (false, AF_INET, NULL);
    NP_ASSERT_TRUE (watch_entities (DYNv4, IFNAME));
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: No ifindex for \"eth99\"\n");
}

void test_entity_new_dynamic_ipv4_no_address ()
{
    NP_TEST_START
    setup_test (true, -1, NULL);
    NP_ASSERT_TRUE (watch_entities (DYNv4, IFNAME));
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("")
}

void test_entity_new_dynamic_ipv4 ()
{
    NP_TEST_START
    setup_test (true, AF_INET, NULL);
    NP_ASSERT_TRUE (watch_entities (DYNv4, IFNAME));
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP4ADDR "_32");
    NP_ASSERT_STR_EQUAL (apteryx_value, IP4ADDR "/32");
    NP_TEST_END ("")
}

void test_entity_del_dynamic_ipv4 ()
{
    NP_TEST_START
    setup_test (true, AF_INET, NULL);
    watch_entities (DYNv4, IFNAME);
    free (apteryx_path);
    apteryx_path = NULL;
    free (apteryx_value);
    apteryx_value = NULL;
    NP_ASSERT_TRUE (watch_entities (DYNv4, NULL));
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP4ADDR "_32");
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("")
}

void test_entity_action_invalid ()
{
    NP_TEST_START
    setup_test (true, -1, "invalid address cb action");
    struct nl_object *addr = make_addr (AF_INET, IP4ADDR);
    nl_addr_cb (-1, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: invalid address cb action:-1\n");
}

void test_entity_addr_null ()
{
    NP_TEST_START
    setup_test (true, -1, "missing address object");
    nl_addr_cb (NL_ACT_NEW, NULL, NULL);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: missing address object\n");
}

void test_entity_addr_no_family ()
{
    NP_TEST_START
    setup_test (true, -1, "invalid address object");
    struct nl_object *addr = make_addr (AF_INET, IP4ADDR);
    rtnl_addr_set_family ((struct rtnl_addr *) addr, -1);
    nl_addr_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: invalid address object\n");
}

void test_entity_addr_no_interface ()
{
    NP_TEST_START
    setup_test (true, -1, "invalid address object");
    struct nl_object *addr = make_addr (AF_INET, IP4ADDR);
    rtnl_addr_set_ifindex ((struct rtnl_addr *) addr, 0);
    nl_addr_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: invalid address object\n");
}

void test_entity_addr_no_address ()
{
    NP_TEST_START
    setup_test (true, -1, "invalid address object");
    struct nl_object *addr = make_addr (AF_INET, NULL);
    nl_addr_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: invalid address object\n");
}

void test_entity_dynamic_ipv4_new_address ()
{
    NP_TEST_START
    setup_test (true, -1, NULL);
    watch_entities (DYNv4, IFNAME);
    struct nl_object *addr = make_addr (AF_INET, IP4ADDR);
    nl_addr_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP4ADDR "_32");
    NP_ASSERT_STR_EQUAL (apteryx_value, IP4ADDR "/32");
    NP_TEST_END ("")
}

void test_entity_dynamic_ipv4_del_address ()
{
    NP_TEST_START
    setup_test (true, AF_INET, NULL);
    watch_entities (DYNv4, IFNAME);
    free (apteryx_path);
    apteryx_path = NULL;
    free (apteryx_value);
    apteryx_value = NULL;
    struct nl_object *addr = make_addr (AF_INET, IP4ADDR);
    nl_addr_cb (NL_ACT_DEL, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP4ADDR "_32");
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("")
}

void test_entity_new_dynamic_ipv6_no_interface ()
{
    NP_TEST_START
    setup_test (false, AF_INET6, NULL);
    NP_ASSERT_TRUE (watch_entities (DYNv6, IFNAME));
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("ENTITY: No ifindex for \"eth99\"\n");
}

void test_entity_new_dynamic_ipv6_no_address ()
{
    NP_TEST_START
    setup_test (true, -1, NULL);
    NP_ASSERT_TRUE (watch_entities (DYNv6, IFNAME));
    NP_ASSERT_NULL (apteryx_path);
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("")
}

void test_entity_new_dynamic_ipv6 ()
{
    NP_TEST_START
    setup_test (true, AF_INET6, NULL);
    NP_ASSERT_TRUE (watch_entities (DYNv6, IFNAME));
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP6ADDR "_128");
    NP_ASSERT_STR_EQUAL (apteryx_value, IP6ADDR "/128");
    NP_TEST_END ("")
}

void test_entity_del_dynamic_ipv6 ()
{
    NP_TEST_START
    setup_test (true, AF_INET6, NULL);
    watch_entities (DYNv6, IFNAME);
    free (apteryx_path);
    apteryx_path = NULL;
    free (apteryx_value);
    apteryx_value = NULL;
    NP_ASSERT_TRUE (watch_entities (DYNv6, NULL));
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP6ADDR "_128");
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("")
}

void test_entity_dynamic_ipv6_new_address ()
{
    NP_TEST_START
    setup_test (true, -1, NULL);
    watch_entities (DYNv6, IFNAME);
    struct nl_object *addr = make_addr (AF_INET6, IP6ADDR);
    nl_addr_cb (NL_ACT_NEW, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP6ADDR "_128");
    NP_ASSERT_STR_EQUAL (apteryx_value, IP6ADDR "/128");
    NP_TEST_END ("")
}

void test_entity_dynamic_ipv6_del_address ()
{
    NP_TEST_START
    setup_test (true, AF_INET6, NULL);
    watch_entities (DYNv6, IFNAME);
    free (apteryx_path);
    apteryx_path = NULL;
    free (apteryx_value);
    apteryx_value = NULL;
    struct nl_object *addr = make_addr (AF_INET6, IP6ADDR);
    nl_addr_cb (NL_ACT_DEL, NULL, addr);
    nl_object_put (addr);
    NP_ASSERT_STR_EQUAL (apteryx_path, HOST "/subnets/dynamic_" IP6ADDR "_128");
    NP_ASSERT_NULL (apteryx_value);
    NP_TEST_END ("")
}
