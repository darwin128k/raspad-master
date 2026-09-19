#include <gtest/gtest.h>

#include <lh/net/ip.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/util/addr.h>
#include <raspad/master/registry.h>

namespace
{

lh_net_ip4_socket_addr_t
Addr(lh_u8_t a, lh_u8_t b, lh_u8_t c, lh_u8_t d, lh_net_port_t port)
{
    lh_net_ip4_t ip = lh_net_ip4_make(a, b, c, d);
    return lh_net_ip4_socket_addr_make(&ip, port);
}

TEST(raspad_master_registry, add_get_count)
{
    raspad_master_registry_t registry;
    raspad_master_registry_init(&registry, 8);
    lh_net_ip4_socket_addr_t addr = Addr(1, 2, 3, 4, 27015);
    ASSERT_TRUE(raspad_master_registry_add(&registry, &addr));
    EXPECT_EQ(raspad_master_registry_get_size(&registry), 1U);

    lh_net_ip4_socket_addr_t got{};
    ASSERT_TRUE(raspad_master_registry_get(&registry, 0, &got));
    EXPECT_TRUE(lh_net_ip4_socket_addr_equals(&got, &addr));

    ASSERT_TRUE(raspad_master_registry_add(&registry, &addr));
    EXPECT_EQ(raspad_master_registry_get_size(&registry), 1U);
    raspad_master_registry_deinit(&registry);
}

TEST(raspad_master_registry, respects_cap)
{
    raspad_master_registry_t registry;
    raspad_master_registry_init(&registry, 1);
    lh_net_ip4_socket_addr_t a = Addr(1, 1, 1, 1, 1);
    lh_net_ip4_socket_addr_t b = Addr(2, 2, 2, 2, 2);
    ASSERT_TRUE(raspad_master_registry_add(&registry, &a));
    EXPECT_FALSE(raspad_master_registry_add(&registry, &b));
    raspad_master_registry_deinit(&registry);
}

} // namespace
