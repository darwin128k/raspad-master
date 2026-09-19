/**
 * @file config.h
 * @brief Bind, flood/size limits, and path to the hand-edited server list.
 *
 * Application policy, not lh. UDP 27010 is the GoldSrc/CS master query port.
 * The list comes from servers.json (edit and save; no rebuild).
 */

#ifndef RASPAD_MASTER_CONFIG_H
#define RASPAD_MASTER_CONFIG_H

#include <lh/char.h>
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
 * @def RASPAD_MASTER_LIST_PATH
 * @brief Default list file, relative to the process working directory.
 */
#define RASPAD_MASTER_LIST_PATH "servers.json"

/**
 * @struct raspad_master_config
 * @typedef raspad_master_config_t
 * @brief Tunables for one master process.
 */
struct raspad_master_config
{
    lh_net_ip4_t bind_ip;
    lh_net_port_t udp_port;
    lh_char_t list_path[256];
    lh_u32_t flood_max_hits;
    lh_u64_t flood_window_ms;
    lh_usize_t max_udp_bytes;
    lh_usize_t max_servers;
};
typedef struct raspad_master_config raspad_master_config_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Default: UDP `0.0.0.0:27010`, list `servers.json`, 30 hits / 1s.
 */
raspad_master_config_t
raspad_master_config_make_default(void);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_CONFIG_H */
