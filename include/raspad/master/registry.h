/**
 * @file registry.h
 * @brief In-memory list of registered game servers (IP + port).
 *
 * No sockets, no protocol. The UDP query handler reads this.
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
    lh_vector_t servers;
    lh_usize_t max_servers;
};
typedef struct raspad_master_registry raspad_master_registry_t;

LH_COMPILER_EXTERN_C_BEGIN

void
raspad_master_registry_init(raspad_master_registry_t *self, lh_usize_t max_servers);

void
raspad_master_registry_deinit(raspad_master_registry_t *self);

lh_usize_t
raspad_master_registry_get_size(const raspad_master_registry_t *self);

/**
 * @brief Drop every entry. Capacity is kept.
 */
void
raspad_master_registry_clear(raspad_master_registry_t *self);

/**
 * @brief Insert @p addr if it is not already present.
 *
 * @return ::lh_bool_true if the list contains @p addr afterwards (added or
 *         already there), ::lh_bool_false if the cap was reached.
 */
lh_bool_t
raspad_master_registry_add(raspad_master_registry_t *self, const lh_net_ip4_socket_addr_t *addr);

lh_bool_t
raspad_master_registry_remove(raspad_master_registry_t *self,
                              const lh_net_ip4_socket_addr_t *addr);

lh_bool_t
raspad_master_registry_get(const raspad_master_registry_t *self, lh_usize_t index,
                           lh_net_ip4_socket_addr_t *out);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_REGISTRY_H */
