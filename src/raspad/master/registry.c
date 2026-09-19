#include <raspad/master/registry.h>
#include <lh/assert.h>
#include <lh/null.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/util/type.h>

void
raspad_master_registry_init(raspad_master_registry_t *self, lh_usize_t max_servers)
{
    lh_assert_runtime_ref(self);
    lh_vector_init(lh_addr_of(self->servers), lh_type_size(lh_net_ip4_socket_addr_t));
    self->max_servers = max_servers;
}

void
raspad_master_registry_deinit(raspad_master_registry_t *self)
{
    lh_assert_runtime_ref(self);
    lh_vector_deinit(lh_addr_of(self->servers));
    self->max_servers = 0;
}

lh_usize_t
raspad_master_registry_get_size(const raspad_master_registry_t *self)
{
    lh_assert_runtime_ref(self);
    return lh_vector_get_size(lh_addr_of(self->servers));
}

static lh_bool_t
raspad_master_registry_find(const raspad_master_registry_t *self,
                            const lh_net_ip4_socket_addr_t *addr, lh_usize_t *index)
{
    lh_usize_t i;
    lh_usize_t n;

    n = raspad_master_registry_get_size(self);
    for (i = 0; i < n; ++i)
    {
        const lh_net_ip4_socket_addr_t *slot =
            lh_ptr_ccast(lh_net_ip4_socket_addr_t, lh_vector_get_ptr(lh_addr_of(self->servers), i));
        if (lh_net_ip4_socket_addr_equals(slot, addr))
        {
            if (lh_null_ne(index))
            {
                *index = i;
            }
            return lh_bool_true;
        }
    }
    return lh_bool_false;
}

lh_bool_t
raspad_master_registry_add(raspad_master_registry_t *self, const lh_net_ip4_socket_addr_t *addr)
{
    lh_assert_runtime_ref(self);
    lh_assert_runtime_ref(addr);

    if (raspad_master_registry_find(self, addr, lh_null))
    {
        return lh_bool_true;
    }
    if (raspad_master_registry_get_size(self) >= self->max_servers)
    {
        return lh_bool_false;
    }
    lh_vector_push_back(lh_addr_of(self->servers), addr);
    return lh_bool_true;
}

lh_bool_t
raspad_master_registry_remove(raspad_master_registry_t *self, const lh_net_ip4_socket_addr_t *addr)
{
    lh_usize_t index;

    lh_assert_runtime_ref(self);
    lh_assert_runtime_ref(addr);
    if (!raspad_master_registry_find(self, addr, lh_addr_of(index)))
    {
        return lh_bool_false;
    }
    lh_vector_erase(lh_addr_of(self->servers), index, lh_null);
    return lh_bool_true;
}

lh_bool_t
raspad_master_registry_get(const raspad_master_registry_t *self, lh_usize_t index,
                           lh_net_ip4_socket_addr_t *out)
{
    const lh_net_ip4_socket_addr_t *slot;

    lh_assert_runtime_ref(self);
    lh_assert_runtime_ref(out);
    if (!lh_vector_is_valid_index(lh_addr_of(self->servers), index))
    {
        return lh_bool_false;
    }
    slot = lh_ptr_ccast(lh_net_ip4_socket_addr_t, lh_vector_get_ptr(lh_addr_of(self->servers), index));
    *out = *slot;
    return lh_bool_true;
}
