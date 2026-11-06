#include <gtest/gtest.h>

#include <lh/core/net/ip.h>
#include <lh/core/net/socket/addr/ip4.h>
#include <lh/os/net.h>
#include <lh/os/net/socket.h>
#include <raspad/master.h>
#include <raspad/master/config.h>

namespace
{

class RaspadMasterTest : public ::testing::Test
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

TEST_F(RaspadMasterTest, bound_port_is_not_shared)
{
    /* The master owns its query port: a second socket, even one asking for
       SO_REUSEADDR, must not bind next to it and read the queries. */
    raspad_master_config_t config = raspad_master_config_make_default();
    lh_net_ip4_init(&config.bind_ip, 127, 0, 0, 1);
    config.udp_port = 0;
    config.list_path[0] = '\0';
    raspad_master_t master;
    raspad_master_init(&master, &config, nullptr);
    ASSERT_TRUE(raspad_master_bind(&master));
    lh_net_ip4_socket_addr_t address{};
    ASSERT_TRUE(lh_os_net_socket_get_local_addr(&master.udp, &address));

    lh_os_net_socket_t thief;
    lh_os_net_socket_init(&thief);
    ASSERT_TRUE(lh_os_net_socket_open(&thief, lh_os_system_net_socket_type_udp));
    ASSERT_TRUE(lh_os_net_socket_set_reuse_addr(&thief, lh_bool_true));
    EXPECT_FALSE(lh_os_net_socket_bind(&thief, &address));

    lh_os_net_socket_close(&thief);
    raspad_master_deinit(&master);
}

} // namespace
