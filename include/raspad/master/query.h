/**
 * @file query.h
 * @brief GoldSrc/CS master list query (`0x31`) and reply (`FF FF FF FF 66 0A`).
 *
 * Pure codec: bytes in, bytes out. No sockets.
 */

#ifndef RASPAD_MASTER_QUERY_H
#define RASPAD_MASTER_QUERY_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/ptr.h>
#include <lh/size.h>

/**
 * @def RASPAD_MASTER_QUERY_OPCODE
 * @brief A2M_GET_SERVERS_BATCH2 (`'1'`).
 */
#define RASPAD_MASTER_QUERY_OPCODE 0x31U

/**
 * @def RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE
 * @brief `FF FF FF FF 66 0A`.
 */
#define RASPAD_MASTER_QUERY_REPLY_HEADER_SIZE 6U

/**
 * @def RASPAD_MASTER_QUERY_ADDR_SIZE
 * @brief IPv4 (4) + port (2).
 */
#define RASPAD_MASTER_QUERY_ADDR_SIZE 6U

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief True when @p buf is a list query (starts with ::RASPAD_MASTER_QUERY_OPCODE).
 *
 * @param buf  Datagram bytes.
 * @param size Datagram length.
 * @return ::lh_bool_true if the first byte is `'1'`.
 */
lh_bool_t
raspad_master_query_is_list(const lh_ptr buf, lh_usize_t size);

/**
 * @brief Write a GoldSrc server-list reply for @p servers[0..count).
 *
 * Truncates the list if @p out_size cannot hold every entry plus terminator.
 * Always ends with `0.0.0.0:0` when there is room for the terminator.
 *
 * @param out      Destination buffer.
 * @param out_size Capacity of @p out.
 * @param servers  May be null when @p count is `0`.
 * @param count    Number of entries in @p servers.
 * @return Bytes written, or 0 if @p out_size is too small for the header
 *         and terminator.
 */
lh_usize_t
raspad_master_query_write_reply(lh_ptr out, lh_usize_t out_size,
                                const lh_net_ip4_socket_addr_t *servers, lh_usize_t count);

/**
 * @brief Read one `IP:port` at @p index from a reply written by
 *        ::raspad_master_query_write_reply.
 *
 * @param buf   Reply bytes.
 * @param size  Reply length.
 * @param index `0` is the first server after the header.
 * @param out   Destination.
 * @return ::lh_bool_true when the slot is a real server (not the terminator).
 */
lh_bool_t
raspad_master_query_read_reply_addr(const lh_ptr buf, lh_usize_t size, lh_usize_t index,
                                    lh_net_ip4_socket_addr_t *out);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_QUERY_H */
