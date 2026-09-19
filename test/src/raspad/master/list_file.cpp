#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#include <lh/net/ip.h>
#include <lh/net/socket/addr/ip4.h>
#include <raspad/master/list_file.h>
#include <raspad/master/registry.h>

namespace
{

TEST(raspad_master_list_file, loads_servers_object)
{
    const char *path = "raspad_list_file_test.json";
    FILE *f = std::fopen(path, "wb");
    ASSERT_TRUE(f != nullptr);
    const char json[] =
        "{\"servers\":[{\"ip\":\"10.0.0.1\",\"port\":27015},{\"ip\":\"10.0.0.2\",\"port\":27016}]}";
    std::fwrite(json, 1, sizeof(json) - 1, f);
    std::fclose(f);

    raspad_master_registry_t registry;
    raspad_master_registry_init(&registry, 8);
    ASSERT_TRUE(raspad_master_list_file_load(path, &registry));
    EXPECT_EQ(raspad_master_registry_get_size(&registry), 2U);
    lh_net_ip4_t ip = lh_net_ip4_make(10, 0, 0, 1);
    lh_net_ip4_socket_addr_t want = lh_net_ip4_socket_addr_make(&ip, 27015);
    lh_net_ip4_socket_addr_t got{};
    ASSERT_TRUE(raspad_master_registry_get(&registry, 0, &got));
    EXPECT_TRUE(lh_net_ip4_socket_addr_equals(&got, &want));
    raspad_master_registry_deinit(&registry);
    std::remove(path);
}

TEST(raspad_master_list_file, rejects_bad_json)
{
    const char *path = "raspad_list_file_bad.json";
    FILE *f = std::fopen(path, "wb");
    ASSERT_TRUE(f != nullptr);
    const char json[] = "{not json";
    std::fwrite(json, 1, sizeof(json) - 1, f);
    std::fclose(f);

    raspad_master_registry_t registry;
    raspad_master_registry_init(&registry, 8);
    lh_net_ip4_t ip = lh_net_ip4_make(1, 2, 3, 4);
    lh_net_ip4_socket_addr_t keep = lh_net_ip4_socket_addr_make(&ip, 1);
    ASSERT_TRUE(raspad_master_registry_add(&registry, &keep));
    EXPECT_FALSE(raspad_master_list_file_load(path, &registry));
    EXPECT_EQ(raspad_master_registry_get_size(&registry), 1U);
    raspad_master_registry_deinit(&registry);
    std::remove(path);
}

} // namespace
