#include <gtest/gtest.h>

#include <lh/net/ip.h>
#include <lh/net/socket/addr/ip4.h>
#include <raspad/master/query.h>

namespace
{

TEST(raspad_master_query, rejects_empty_and_wrong_opcode)
{
    unsigned char empty[] = {0};
    EXPECT_FALSE(raspad_master_query_is_list(nullptr, 0));
    empty[0] = 0x32;
    EXPECT_FALSE(raspad_master_query_is_list(empty, 1));
    empty[0] = 0x31;
    EXPECT_TRUE(raspad_master_query_is_list(empty, 1));
}

TEST(raspad_master_query, write_reply_roundtrips_one_server)
{
    lh_net_ip4_t ip = lh_net_ip4_make(192, 168, 0, 1);
    lh_net_ip4_socket_addr_t servers[1] = {lh_net_ip4_socket_addr_make(&ip, 27015)};
    unsigned char buf[64] = {};
    lh_usize_t n = raspad_master_query_write_reply(buf, sizeof(buf), servers, 1);
    ASSERT_EQ(n, RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE + RASPAD_MASTER_QUERY_ADDR_SIZE * 2U);
    EXPECT_EQ(buf[4], 0x66);
    EXPECT_EQ(buf[5], 0x0A);

    lh_net_ip4_socket_addr_t got{};
    ASSERT_TRUE(raspad_master_query_read_reply_addr(buf, n, 0, &got));
    EXPECT_TRUE(lh_net_ip4_socket_addr_equals(&got, &servers[0]));
    EXPECT_FALSE(raspad_master_query_read_reply_addr(buf, n, 1, &got));
}

} // namespace
