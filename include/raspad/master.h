/**
 * @file master.h
 * @brief Process-facing master: registry + flood + UDP 27010 (read-only).
 */

#ifndef RASPAD_MASTER_H
#define RASPAD_MASTER_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/logger.h>
#include <lh/os/net/socket.h>
#include <raspad/master/config.h>
#include <raspad/master/flood.h>
#include <raspad/master/registry.h>

/**
 * @struct raspad_master
 * @typedef raspad_master_t
 * @brief Bound UDP socket plus the shared server list.
 */
struct raspad_master
{
    raspad_master_config_t config;
    raspad_master_registry_t registry;
    raspad_master_flood_t flood;
    lh_logger_t *logger;
    lh_os_net_socket_t udp;
};
typedef struct raspad_master raspad_master_t;

LH_COMPILER_EXTERN_C_BEGIN

void
raspad_master_init(raspad_master_t *self, const raspad_master_config_t *config,
                   lh_logger_t *logger);

void
raspad_master_deinit(raspad_master_t *self);

/**
 * @brief Bind the UDP query socket from @p self's config.
 */
lh_bool_t
raspad_master_bind(raspad_master_t *self);

/**
 * @brief Service ready sockets once. @p timeout_ms `0` polls.
 *
 * @return ::lh_bool_true if select ran (even with no events).
 */
lh_bool_t
raspad_master_poll(raspad_master_t *self, lh_u64_t timeout_ms);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_H */
