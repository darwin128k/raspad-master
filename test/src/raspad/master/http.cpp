#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include <lh/net/ip.h>
#include <lh/net/socket/addr/ip4.h>
#include <raspad/master/http.h>
#include <raspad/master/registry.h>

namespace
{

TEST(raspad_master_http, parses_get_path)
{
    const char req[] = "GET /health HTTP/1.1\r\nHost: x\r\n\r\n";
    raspad_master_http_request_t parsed{};
    ASSERT_TRUE(raspad_master_http_request_parse(req, sizeof(req) - 1, &parsed));
    EXPECT_EQ(parsed.method, raspad_master_http_method_get);
    EXPECT_EQ(std::string(parsed.path, parsed.path_size), "/health");
}

TEST(raspad_master_http, health_and_servers_json)
{
    raspad_master_registry_t registry;
    raspad_master_registry_init(&registry, 8);
    lh_net_ip4_t ip = lh_net_ip4_make(127, 0, 0, 1);
    lh_net_ip4_socket_addr_t addr = lh_net_ip4_socket_addr_make(&ip, 27015);
    ASSERT_TRUE(raspad_master_registry_add(&registry, &addr));

    const char health[] = "GET /health HTTP/1.1\r\n\r\n";
    raspad_master_http_request_t req{};
    ASSERT_TRUE(raspad_master_http_request_parse(health, sizeof(health) - 1, &req));
    char out[512] = {};
    lh_usize_t n = raspad_master_http_write_response(out, sizeof(out), &registry, &req);
    ASSERT_GT(n, 0U);
    EXPECT_NE(std::string(out, n).find("{\"ok\":true}"), std::string::npos);

    const char list[] = "GET /servers.json HTTP/1.1\r\n\r\n";
    ASSERT_TRUE(raspad_master_http_request_parse(list, sizeof(list) - 1, &req));
    n = raspad_master_http_write_response(out, sizeof(out), &registry, &req);
    ASSERT_GT(n, 0U);
    std::string body(out, n);
    EXPECT_NE(body.find("127.0.0.1"), std::string::npos);
    EXPECT_NE(body.find("27015"), std::string::npos);

    raspad_master_registry_deinit(&registry);
}

TEST(raspad_master_http, post_registers_server)
{
    raspad_master_registry_t registry;
    raspad_master_registry_init(&registry, 8);
    const char post[] =
        "POST /servers.json HTTP/1.1\r\nContent-Length: 32\r\n\r\n{\"ip\":\"10.0.0.2\",\"port\":27016}";
    raspad_master_http_request_t req{};
    ASSERT_TRUE(raspad_master_http_request_parse(post, sizeof(post) - 1, &req));
    char out[256] = {};
    ASSERT_GT(raspad_master_http_write_response(out, sizeof(out), &registry, &req), 0U);
    EXPECT_EQ(raspad_master_registry_get_size(&registry), 1U);
    raspad_master_registry_deinit(&registry);
}

} // namespace
