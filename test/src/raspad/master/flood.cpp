#include <gtest/gtest.h>

#include <lh/net/ip.h>
#include <raspad/master/flood.h>

namespace
{

TEST(raspad_master_flood, allows_until_budget_then_drops)
{
    raspad_master_flood_t flood;
    raspad_master_flood_init(&flood, 2, 1000, 8);
    lh_net_ip4_t ip = lh_net_ip4_make(10, 0, 0, 1);

    EXPECT_TRUE(raspad_master_flood_allow(&flood, &ip, 0));
    EXPECT_TRUE(raspad_master_flood_allow(&flood, &ip, 10));
    EXPECT_FALSE(raspad_master_flood_allow(&flood, &ip, 20));
    EXPECT_TRUE(raspad_master_flood_allow(&flood, &ip, 1000));
    raspad_master_flood_deinit(&flood);
}

TEST(raspad_master_flood, isolates_peers)
{
    raspad_master_flood_t flood;
    raspad_master_flood_init(&flood, 1, 1000, 8);
    lh_net_ip4_t a = lh_net_ip4_make(1, 1, 1, 1);
    lh_net_ip4_t b = lh_net_ip4_make(2, 2, 2, 2);
    EXPECT_TRUE(raspad_master_flood_allow(&flood, &a, 0));
    EXPECT_FALSE(raspad_master_flood_allow(&flood, &a, 1));
    EXPECT_TRUE(raspad_master_flood_allow(&flood, &b, 1));
    raspad_master_flood_deinit(&flood);
}

} // namespace
