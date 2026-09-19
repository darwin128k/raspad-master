/**
 * @file registry.h
 * @brief In-memory list of game servers (IP + port).
 *
 * No sockets, no JSON, no protocol. ::raspad_master_list_file_load writes it;
 * the UDP query handler reads it.
 */

#ifndef RASPAD_MASTER_REGISTRY_H
#define RASPAD_MASTER_REGISTRY_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/size.h>
#include <lh/vector.h>

/**
 * @struct raspad_master_registry
 * @typedef raspad_master_registry_t
 * @brief Vector of ::lh_net_ip4_socket_addr_t with a hard cap.
 */
struct raspad_master_registry
{
    lh_vector_t servers;   /**< Elements are ::lh_net_ip4_socket_addr_t. */
    lh_usize_t max_servers; /**< ::raspad_master_registry_add refuses past this. */
};
typedef struct raspad_master_registry raspad_master_registry_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Empty list with room for up to @p max_servers entries.
 *
 * @param self        Registry to initialise.
 * @param max_servers Hard cap; `0` means nothing can be added.
 */
void
raspad_master_registry_init(raspad_master_registry_t *self, lh_usize_t max_servers);

/**
 * @brief Free storage. @p self must not be used until init again.
 *
 * @param self Registry previously passed to ::raspad_master_registry_init.
 */
void
raspad_master_registry_deinit(raspad_master_registry_t *self);

/**
 * @brief Number of stored addresses.
 *
 * @param self Registry.
 * @return Count in `[0, max_servers]`.
 */
lh_usize_t
raspad_master_registry_get_size(const raspad_master_registry_t *self);

/**
 * @brief Drop every entry. Capacity is kept.
 *
 * @param self Registry.
 */
void
raspad_master_registry_clear(raspad_master_registry_t *self);

/**
 * @brief Insert @p addr if it is not already present.
 *
 * @param self Registry.
 * @param addr Game server `IP:port`.
 * @return ::lh_bool_true if the list contains @p addr afterwards (added or
 *         already there), ::lh_bool_false if the cap was reached.
 */
lh_bool_t
raspad_master_registry_add(raspad_master_registry_t *self, const lh_net_ip4_socket_addr_t *addr);

/**
 * @brief Remove @p addr if present.
 *
 * @param self Registry.
 * @param addr Address to drop.
 * @return ::lh_bool_true if an entry was removed.
 */
lh_bool_t
raspad_master_registry_remove(raspad_master_registry_t *self,
                              const lh_net_ip4_socket_addr_t *addr);

/**
 * @brief Copy the address at @p index into @p out.
 *
 * @param self  Registry.
 * @param index `0 .. size-1`.
 * @param out   Destination.
 * @return ::lh_bool_true if @p index was valid.
 */
lh_bool_t
raspad_master_registry_get(const raspad_master_registry_t *self, lh_usize_t index,
                           lh_net_ip4_socket_addr_t *out);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_REGISTRY_H */
