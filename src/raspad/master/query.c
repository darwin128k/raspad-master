#include <raspad/master/query.h>
#include <lh/assert.h>
#include <lh/cast/static.h>
#include <lh/char.h>
#include <lh/memory.h>
#include <lh/net/ip.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <lh/util/bit/endian.h>
#include <lh/util/ptr.h>

static const lh_uchar_t raspad_master_query_reply_header[RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE] = {
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x66U, 0x0AU};

lh_bool_t
raspad_master_query_is_list(const lh_ptr buf, lh_usize_t size)
{
    const lh_uchar_t *bytes;

    if (lh_null_eq(buf) || size == 0)
    {
        return lh_bool_false;
    }
    bytes = lh_ptr_ccast(lh_uchar_t, buf);
    return lh_cast_static(lh_bool_t, bytes[0] == RASPAD_MASTER_QUERY_OPCODE);
}

static void
raspad_master_query_pack_addr(lh_uchar_t *out, const lh_net_ip4_socket_addr_t *addr)
{
    lh_net_ip4_t ip = lh_net_ip4_socket_addr_get_ip(addr);
    lh_net_port_t port = lh_net_ip4_socket_addr_get_port(addr);

    out[0] = lh_net_ip4_get_octet(lh_addr_of(ip), LH_NET_IP4_OCTET_INDEX_0);
    out[1] = lh_net_ip4_get_octet(lh_addr_of(ip), LH_NET_IP4_OCTET_INDEX_1);
    out[2] = lh_net_ip4_get_octet(lh_addr_of(ip), LH_NET_IP4_OCTET_INDEX_2);
    out[3] = lh_net_ip4_get_octet(lh_addr_of(ip), LH_NET_IP4_OCTET_INDEX_3);
    lh_bit_pack_be16(port, out + 4);
}

lh_usize_t
raspad_master_query_write_reply(lh_ptr out, lh_usize_t out_size,
                                const lh_net_ip4_socket_addr_t *servers, lh_usize_t count)
{
    lh_uchar_t *bytes;
    lh_usize_t max_entries;
    lh_usize_t i;
    lh_usize_t off;

    lh_assert_runtime_ref(out);
    if (out_size < RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE + RASPAD_MASTER_QUERY_ADDR_SIZE)
    {
        return 0;
    }
    bytes = lh_ptr_cast(lh_uchar_t, out);
    lh_memory_copy(bytes, RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE, raspad_master_query_reply_header,
                   RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE);
    off = RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE;

    max_entries = (out_size - RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE - RASPAD_MASTER_QUERY_ADDR_SIZE) /
                  RASPAD_MASTER_QUERY_ADDR_SIZE;
    if (count > max_entries)
    {
        count = max_entries;
    }
    if (count != 0)
    {
        lh_assert_runtime_ref(servers);
    }
    for (i = 0; i < count; ++i)
    {
        raspad_master_query_pack_addr(bytes + off, lh_addr_of(servers[i]));
        off += RASPAD_MASTER_QUERY_ADDR_SIZE;
    }
    lh_memory_set(bytes + off, RASPAD_MASTER_QUERY_ADDR_SIZE, 0);
    off += RASPAD_MASTER_QUERY_ADDR_SIZE;
    return off;
}

lh_bool_t
raspad_master_query_read_reply_addr(const lh_ptr buf, lh_usize_t size, lh_usize_t index,
                                    lh_net_ip4_socket_addr_t *out)
{
    const lh_uchar_t *bytes;
    lh_usize_t off;
    lh_net_ip4_t ip;
    lh_net_port_t port;

    lh_assert_runtime_ref(buf);
    lh_assert_runtime_ref(out);
    off = RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE + index * RASPAD_MASTER_QUERY_ADDR_SIZE;
    if (size < off + RASPAD_MASTER_QUERY_ADDR_SIZE)
    {
        return lh_bool_false;
    }
    bytes = lh_ptr_ccast(lh_uchar_t, buf) + off;
    if (bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0 && bytes[4] == 0 &&
        bytes[5] == 0)
    {
        return lh_bool_false;
    }
    ip = lh_net_ip4_make(bytes[0], bytes[1], bytes[2], bytes[3]);
    port = lh_bit_unpack_be16(bytes + 4);
    *out = lh_net_ip4_socket_addr_make(lh_addr_of(ip), port);
    return lh_bool_true;
}
