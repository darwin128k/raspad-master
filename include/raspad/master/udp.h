/**
 * @file udp.h
 * @brief One datagram in → optional GoldSrc list reply out.
 *
 * Uses ::lh_io_dgram_t so the handler does not name a socket.
 */

#ifndef RASPAD_MASTER_UDP_H
#define RASPAD_MASTER_UDP_H

#include <lh/compiler/extern/c.h>
#include <lh/io/dgram.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/numeric/fixed/types.h>
#include <lh/ptr.h>
#include <lh/size.h>
#include <raspad/master/config.h>
#include <raspad/master/flood.h>
#include <raspad/master/registry.h>

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Handle one UDP datagram from @p peer.
 *
 * Drops oversized packets, over-budget peers, and non-query opcodes.
 */
void
raspad_master_udp_handle(lh_io_dgram_t *dgram, raspad_master_registry_t *registry,
                         raspad_master_flood_t *flood, const lh_ptr buf, lh_usize_t size,
                         const lh_net_ip4_socket_addr_t *peer, lh_u64_t now_ms,
                         const raspad_master_config_t *config);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_UDP_H */
