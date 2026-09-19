/**
 * @file config.h
 * @brief Ports, bind address, and flood/size limits for the master process.
 *
 * Application policy, not lh. Defaults match GoldSrc/CS master (UDP 27010)
 * and a non-privileged HTTP port for the REST list.
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
 * @def RASPAD_MASTER_HTTP_PORT
 * @brief Default REST listen port (HTTP; HTTPS is later).
 */
#define RASPAD_MASTER_HTTP_PORT 8080U

/**
 * @struct raspad_master_config
 * @typedef raspad_master_config_t
 * @brief Tunables for one master process.
 */
struct raspad_master_config
{
    lh_net_ip4_t bind_ip;
    lh_net_port_t udp_port;
    lh_net_port_t http_port;
    lh_u32_t flood_max_hits;
    lh_u64_t flood_window_ms;
    lh_usize_t max_udp_bytes;
    lh_usize_t max_http_bytes;
    lh_usize_t max_servers;
};
typedef struct raspad_master_config raspad_master_config_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Default config: `0.0.0.0:27010` UDP, `:8080` HTTP, 30 hits / 1s.
 */
raspad_master_config_t
raspad_master_config_make_default(void);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_CONFIG_H */
