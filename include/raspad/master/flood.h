/**
 * @file flood.h
 * @brief Per-IPv4 hit window: drop a peer that exceeds N requests per interval.
 *
 * Application policy. Clock comes from ::lh_os_clock_ms (or a test-injected
 * `now_ms`). Unknown opcode / oversized packets are rejected before this.
 */

#ifndef RASPAD_MASTER_FLOOD_H
#define RASPAD_MASTER_FLOOD_H

#include <lh/bool.h>
#include <lh/compiler/extern/c.h>
#include <lh/net/ip.h>
#include <lh/numeric/fixed/types.h>
#include <lh/size.h>
#include <lh/vector.h>

/**
 * @struct raspad_master_flood_slot
 * @brief One peer's current window.
 */
struct raspad_master_flood_slot
{
    lh_net_ip4_t ip;     /**< Peer address. */
    lh_u32_t hits;       /**< Queries in the current window. */
    lh_u64_t window_ms;  /**< Window start (::lh_os_clock_ms). */
};
typedef struct raspad_master_flood_slot raspad_master_flood_slot_t;

/**
 * @struct raspad_master_flood
 * @typedef raspad_master_flood_t
 * @brief Table of ::raspad_master_flood_slot_t plus policy.
 */
struct raspad_master_flood
{
    lh_vector_t slots;     /**< Elements are ::raspad_master_flood_slot_t. */
    lh_u32_t max_hits;     /**< Allowed hits per ::window_ms. */
    lh_u64_t window_ms;    /**< Window length. */
    lh_usize_t max_slots;  /**< Cap; the oldest slot is evicted when full. */
};
typedef struct raspad_master_flood raspad_master_flood_t;

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Empty table with the given budget.
 *
 * @param self      Flood state.
 * @param max_hits  Hits allowed per window.
 * @param window_ms Window length.
 * @param max_slots Peer-slot cap.
 */
void
raspad_master_flood_init(raspad_master_flood_t *self, lh_u32_t max_hits, lh_u64_t window_ms,
                         lh_usize_t max_slots);

/**
 * @brief Free slot storage.
 *
 * @param self Flood state previously passed to ::raspad_master_flood_init.
 */
void
raspad_master_flood_deinit(raspad_master_flood_t *self);

/**
 * @brief Record one request from @p ip at @p now_ms.
 *
 * @param self   Flood state.
 * @param ip     Peer IPv4.
 * @param now_ms Monotonic milliseconds.
 * @return ::lh_bool_true if the request is inside the budget,
 *         ::lh_bool_false if it should be dropped.
 */
lh_bool_t
raspad_master_flood_allow(raspad_master_flood_t *self, const lh_net_ip4_t *ip, lh_u64_t now_ms);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_FLOOD_H */
