#include <raspad/master/flood.h>
#include <lh/core/assert.h>
#include <lh/core/cast/static.h>
#include <lh/core/null.h>
#include <lh/core/util/addr.h>
#include <lh/core/util/ptr.h>
#include <lh/core/util/type.h>

void
raspad_master_flood_init(raspad_master_flood_t *self, lh_u32_t max_hits, lh_u64_t window_ms,
                         lh_usize_t max_slots)
{
    lh_assert_runtime_ref(self);
    lh_array_init(lh_addr_of(self->slots), lh_type_size(raspad_master_flood_slot_t));
    self->max_hits = max_hits;
    self->window_ms = window_ms;
    self->max_slots = max_slots;
}

void
raspad_master_flood_deinit(raspad_master_flood_t *self)
{
    lh_assert_runtime_ref(self);
    lh_array_deinit(lh_addr_of(self->slots));
    self->max_hits = 0;
    self->window_ms = 0;
    self->max_slots = 0;
}

static raspad_master_flood_slot_t *
raspad_master_flood_find(raspad_master_flood_t *self, const lh_net_ip4_t *ip)
{
    lh_usize_t i;
    lh_usize_t n;

    n = lh_array_get_size(lh_addr_of(self->slots));
    for (i = 0; i < n; ++i)
    {
        raspad_master_flood_slot_t *slot =
            lh_ptr_cast(raspad_master_flood_slot_t, lh_array_get_ptr(lh_addr_of(self->slots), i));
        if (lh_net_ip4_equals(lh_addr_of(slot->ip), ip))
        {
            return slot;
        }
    }
    return lh_null;
}

lh_bool_t
raspad_master_flood_allow(raspad_master_flood_t *self, const lh_net_ip4_t *ip, lh_u64_t now_ms)
{
    raspad_master_flood_slot_t *slot;
    raspad_master_flood_slot_t fresh;

    lh_assert_runtime_ref(self);
    lh_assert_runtime_ref(ip);

    slot = raspad_master_flood_find(self, ip);
    if (lh_null_eq(slot))
    {
        if (lh_array_get_size(lh_addr_of(self->slots)) >= self->max_slots)
        {
            lh_array_erase(lh_addr_of(self->slots), 0, lh_null);
        }
        fresh.ip = *ip;
        fresh.hits = 1U;
        fresh.window_ms = now_ms;
        lh_array_push_back(lh_addr_of(self->slots), lh_addr_of(fresh));
        return lh_bool_true;
    }
    if (now_ms - slot->window_ms >= self->window_ms)
    {
        slot->window_ms = now_ms;
        slot->hits = 1U;
        return lh_bool_true;
    }
    slot->hits += 1U;
    return lh_cast_static(lh_bool_t, slot->hits <= self->max_hits);
}
