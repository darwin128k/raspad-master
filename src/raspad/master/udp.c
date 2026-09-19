#include <raspad/master/udp.h>
#include <lh/assert.h>
#include <lh/char.h>
#include <lh/net/ip.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <raspad/master/query.h>

void
raspad_master_udp_handle(lh_io_dgram_t *dgram, raspad_master_registry_t *registry,
                         raspad_master_flood_t *flood, const lh_ptr buf, lh_usize_t size,
                         const lh_net_ip4_socket_addr_t *peer, lh_u64_t now_ms,
                         const raspad_master_config_t *config)
{
    lh_net_ip4_t peer_ip;
    lh_uchar_t reply[1400];
    lh_net_ip4_socket_addr_t servers[1024];
    lh_usize_t count;
    lh_usize_t i;
    lh_usize_t written;

    lh_assert_runtime_ref(dgram);
    lh_assert_runtime_ref(registry);
    lh_assert_runtime_ref(flood);
    lh_assert_runtime_ref(peer);
    lh_assert_runtime_ref(config);

    if (size == 0 || size > config->max_udp_bytes)
    {
        return;
    }
    peer_ip = lh_net_ip4_socket_addr_get_ip(peer);
    if (!raspad_master_flood_allow(flood, lh_addr_of(peer_ip), now_ms))
    {
        return;
    }
    if (!raspad_master_query_is_list(buf, size))
    {
        return;
    }

    count = raspad_master_registry_get_size(registry);
    if (count > 1024U)
    {
        count = 1024U;
    }
    for (i = 0; i < count; ++i)
    {
        raspad_master_registry_get(registry, i, lh_addr_of(servers[i]));
    }
    written = raspad_master_query_write_reply(reply, sizeof(reply),
                                              count == 0 ? lh_null : servers, count);
    if (written != 0)
    {
        lh_io_dgram_send(dgram, reply, written, peer);
    }
}
