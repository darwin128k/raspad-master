#include <gtest/gtest.h>

#include <string>

#include <lh/io/dgram.h>
#include <lh/net/ip.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/os/net.h>
#include <lh/os/net/socket.h>
#include <lh/util/addr.h>
#include <raspad/master/config.h>
#include <raspad/master/flood.h>
#include <raspad/master/query.h>
#include <raspad/master/registry.h>
#include <raspad/master/udp.h>

namespace
{

class RaspadMasterUdpTest : public ::testing::Test
{
protected:
    static void
    SetUpTestSuite()
    {
        ASSERT_EQ(lh_os_net_init(), lh_bool_true);
    }

    static void
    TearDownTestSuite()
    {
        lh_os_net_deinit();
    }
};

TEST_F(RaspadMasterUdpTest, query_returns_registered_server)
{
    raspad_master_config_t config = raspad_master_config_make_default();
    raspad_master_registry_t registry;
    raspad_master_flood_t flood;
    raspad_master_registry_init(&registry, 8);
    raspad_master_flood_init(&flood, 30, 1000, 8);

    lh_net_ip4_t game_ip = lh_net_ip4_make(192, 168, 1, 10);
    lh_net_ip4_socket_addr_t game = lh_net_ip4_socket_addr_make(&game_ip, 27015);
    ASSERT_TRUE(raspad_master_registry_add(&registry, &game));

    lh_os_net_socket_t server;
    lh_os_net_socket_t client;
    lh_os_net_socket_init(&server);
    lh_os_net_socket_init(&client);
    ASSERT_TRUE(lh_os_net_socket_open(&server, lh_os_net_socket_type_udp));
    ASSERT_TRUE(lh_os_net_socket_open(&client, lh_os_net_socket_type_udp));

    lh_net_ip4_t loopback = lh_net_ip4_make(127, 0, 0, 1);
    lh_net_ip4_socket_addr_t any = lh_net_ip4_socket_addr_make(&loopback, 0);
    ASSERT_TRUE(lh_os_net_socket_bind(&server, &any));
    ASSERT_TRUE(lh_os_net_socket_bind(&client, &any));

    lh_net_ip4_socket_addr_t server_addr{};
    lh_net_ip4_socket_addr_t client_addr{};
    ASSERT_TRUE(lh_os_net_socket_get_local_addr(&server, &server_addr));
    ASSERT_TRUE(lh_os_net_socket_get_local_addr(&client, &client_addr));

    lh_io_dgram_t client_dgram = lh_os_net_socket_get_dgram(&client);
    unsigned char query[] = {0x31, 0xFF, '0', '.', '0', '.', '0', '.', '0', ':', '0', 0, 0};
    ASSERT_EQ(lh_io_dgram_send(&client_dgram, query, sizeof(query), &server_addr),
              static_cast<lh_ssize_t>(sizeof(query)));

    lh_io_dgram_t server_dgram = lh_os_net_socket_get_dgram(&server);
    unsigned char incoming[256] = {};
    lh_net_ip4_socket_addr_t peer{};
    lh_ssize_t n = lh_io_dgram_recv(&server_dgram, incoming, sizeof(incoming), &peer);
    ASSERT_GT(n, 0);
    raspad_master_udp_handle(&server_dgram, &registry, &flood, incoming, static_cast<lh_usize_t>(n),
                             &peer, 0, &config, nullptr);

    unsigned char reply[256] = {};
    lh_net_ip4_socket_addr_t from{};
    n = lh_io_dgram_recv(&client_dgram, reply, sizeof(reply), &from);
    ASSERT_GT(n, 0);
    lh_net_ip4_socket_addr_t got{};
    ASSERT_TRUE(raspad_master_query_read_reply_addr(reply, static_cast<lh_usize_t>(n), 0, &got));
    EXPECT_TRUE(lh_net_ip4_socket_addr_equals(&got, &game));

    lh_os_net_socket_close(&client);
    lh_os_net_socket_close(&server);
    raspad_master_flood_deinit(&flood);
    raspad_master_registry_deinit(&registry);
}

} // namespace
