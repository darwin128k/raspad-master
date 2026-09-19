#include <raspad/master/udp.h>
#include <lh/assert.h>
#include <lh/cast/static.h>
#include <lh/char.h>
#include <lh/logger.h>
#include <lh/net/ip.h>
#include <lh/null.h>
#include <lh/numeric/types.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <raspad/master/query.h>

static void
raspad_master_udp_peer_text(const lh_net_ip4_socket_addr_t *peer, lh_str_ptr out, lh_usize_t out_size)
{
    lh_usize_t n;

    if (out_size == 0U)
    {
        return;
    }
    n = lh_net_ip4_socket_addr_format(peer, out, out_size - 1U);
    out[n] = '\0';
}

void
raspad_master_udp_handle(lh_io_dgram_t *dgram, raspad_master_registry_t *registry,
                         raspad_master_flood_t *flood, const lh_ptr buf, lh_usize_t size,
                         const lh_net_ip4_socket_addr_t *peer, lh_u64_t now_ms,
                         const raspad_master_config_t *config, lh_logger_t *logger)
{
    lh_net_ip4_t peer_ip;
    lh_uchar_t reply[1400];
    lh_net_ip4_socket_addr_t servers[1024];
    lh_char_t from[LH_NET_IP4_SOCKET_ADDR_TEXT_MAX + 1U];
    const lh_uchar_t *bytes;
    lh_usize_t count;
    lh_usize_t i;
    lh_usize_t written;

    lh_assert_runtime_ref(dgram);
    lh_assert_runtime_ref(registry);
    lh_assert_runtime_ref(flood);
    lh_assert_runtime_ref(peer);
    lh_assert_runtime_ref(config);

    raspad_master_udp_peer_text(peer, from, sizeof(from));

    if (size == 0 || size > config->max_udp_bytes)
    {
        if (lh_null_ne(logger))
        {
            lh_logger_warning(logger, "query from %s dropped oversized bytes=%u max=%u", from,
                              lh_cast_static(lh_uint_t, size),
                              lh_cast_static(lh_uint_t, config->max_udp_bytes));
        }
        return;
    }
    peer_ip = lh_net_ip4_socket_addr_get_ip(peer);
    if (!raspad_master_flood_allow(flood, lh_addr_of(peer_ip), now_ms))
    {
        if (lh_null_ne(logger))
        {
            lh_logger_warning(logger, "query from %s dropped flood", from);
        }
        return;
    }
    if (!raspad_master_query_is_list(buf, size))
    {
        bytes = lh_ptr_ccast(lh_uchar_t, buf);
        if (lh_null_ne(logger))
        {
            lh_logger_notice(logger, "query from %s dropped opcode=0x%02X bytes=%u", from,
                             lh_cast_static(lh_uint_t, bytes[0]), lh_cast_static(lh_uint_t, size));
        }
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
    if (lh_null_ne(logger))
    {
        lh_logger_info(logger, "query from %s list reply servers=%u bytes_in=%u bytes_out=%u", from,
                       lh_cast_static(lh_uint_t, count), lh_cast_static(lh_uint_t, size),
                       lh_cast_static(lh_uint_t, written));
    }
}
