/**
 * @file config.h
 * @brief Bind, flood/size limits, and the baked read-only server list seed.
 *
 * Application policy, not lh. UDP 27010 is the GoldSrc/CS master query port.
 * There is no HTTP and no heartbeat: the list is seeded at process start.
 */

#ifndef RASPAD_MASTER_CONFIG_H
#define RASPAD_MASTER_CONFIG_H

#include <lh/compiler/extern/c.h>
#include <lh/net/ip.h>
#include <lh/net/port.h>
#include <lh/numeric/fixed/types.h>
#include <lh/size.h>

/**
 * @def RASPAD_MASTER_UDP_PORT
 * @brief Default GoldSrc/CS master query port.
 */
#define RASPAD_MASTER_UDP_PORT 27010U

/**
 * @def RASPAD_MASTER_SEED_PORT
 * @brief Baked game-server port listed on startup. `0` skips the seed.
 */
#define RASPAD_MASTER_SEED_PORT 27015U

/**
 * @struct raspad_master_config
 * @typedef raspad_master_config_t
 * @brief Tunables for one master process.
 */
struct raspad_master_config
{
    lh_net_ip4_t bind_ip;
    lh_net_port_t udp_port;
    lh_net_ip4_t seed_ip;
    lh_net_port_t seed_port;
    lh_u32_t flood_max_hits;
    lh_u64_t flood_window_ms;
    lh_usize_t max_udp_bytes;
    lh_usize_t max_servers;
};
typedef struct raspad_master_config raspad_master_config_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Default: UDP `0.0.0.0:27010`, seed `37.230.210.218:27015`, 30 hits / 1s.
 */
raspad_master_config_t
raspad_master_config_make_default(void);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_CONFIG_H */
