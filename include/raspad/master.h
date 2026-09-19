/**
 * @file master.h
 * @brief Process-facing master: registry + flood + UDP 27010 (read-only).
 *
 * Loads ::raspad_master_config::list_path (`servers.json`) into memory and
 * answers GoldSrc/CS list queries. No HTTP, no heartbeat. The web panel can
 * later write the same JSON file.
 */

#ifndef RASPAD_MASTER_H
#define RASPAD_MASTER_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/logger.h>
#include <lh/numeric/fixed/types.h>
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
    raspad_master_config_t config;     /**< Copy of the tunables passed to init. */
    raspad_master_registry_t registry; /**< In-memory IP:port list served on UDP. */
    raspad_master_flood_t flood;       /**< Per-IP query budget. */
    lh_logger_t *logger;               /**< Optional; not owned. */
    lh_os_net_socket_t udp;            /**< Bound datagram socket. */
    lh_s64_t list_mtime;               /**< Last applied `servers.json` mtime, or 0. */
};
typedef struct raspad_master raspad_master_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Zero sockets, init registry/flood, load `servers.json` if present.
 *
 * @param self   Master to initialise.
 * @param config Copied into @p self. Must outlive the call only.
 * @param logger Optional sink for list reload lines; may be null.
 */
void
raspad_master_init(raspad_master_t *self, const raspad_master_config_t *config,
                   lh_logger_t *logger);

/**
 * @brief Close the UDP socket and free registry/flood storage.
 *
 * @param self Master previously passed to ::raspad_master_init.
 */
void
raspad_master_deinit(raspad_master_t *self);

/**
 * @brief Bind the UDP query socket from @p self's config.
 *
 * @param self Initialised master.
 * @return ::lh_bool_true on success.
 */
lh_bool_t
raspad_master_bind(raspad_master_t *self);

/**
 * @brief Reload `servers.json` if its mtime changed, then wait for one UDP event.
 *
 * @param self        Bound master.
 * @param timeout_ms  `select` timeout; `0` polls.
 * @return ::lh_bool_true if select ran (even with no events).
 */
lh_bool_t
raspad_master_poll(raspad_master_t *self, lh_u64_t timeout_ms);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_H */
