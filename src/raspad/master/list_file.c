#include <raspad/master/list_file.h>
#include <lh/core/assert.h>
#include <lh/core/cast/static.h>
#include <lh/core/memory.h>
#include <lh/core/memory/allocator.h>
#include <lh/core/net/ip.h>
#include <lh/core/net/port.h>
#include <lh/core/net/socket/addr/ip4.h>
#include <lh/core/null.h>
#include <lh/os/fs/file.h>
#include <lh/os/fs/path.h>
#include <lh/os/fs/stat.h>
#include <lh/os/system/fs/path.h>
#include <lh/core/str/ptr.h>
#include <lh/core/util/addr.h>
#include <lh/core/runtime/allocator.h>
#include <lh/core/web/json/node.h>
#include <lh/core/web/json/reader.h>

#define RASPAD_MASTER_LIST_FILE_MAX 65536U

lh_s64_t
raspad_master_list_file_mtime(const lh_char_t *path)
{
    lh_s64_t mtime;

    if (lh_null_eq(path) || path[0] == '\0')
    {
        return 0;
    }
    lh_fs_path_t parsed;
    lh_str_view_t view;
    lh_fs_path_init(&parsed);
    lh_str_view_init(&view, path);
    lh_fs_path_set(&parsed, view, lh_os_system_fs_path_style_native());
    lh_bool_t ok = lh_os_fs_mtime(&parsed, &mtime);
    lh_fs_path_deinit(&parsed);
    return ok ? mtime : 0;
}

lh_bool_t
raspad_master_list_file_read(const lh_char_t *path, lh_char_t *buf, lh_usize_t capacity,
                             lh_usize_t *size)
{
    lh_os_fs_file_t file;
    lh_usize_t used = 0U;
    lh_bool_t ok = lh_bool_false;

    lh_os_fs_file_init(lh_addr_of(file));
    if (lh_os_fs_file_open_cstr(lh_addr_of(file), path, lh_fs_file_mode_read))
    {
        for (;;)
        {
            lh_char_t probe;
            lh_ssize_t got;

            if (used < capacity)
            {
                got = lh_os_fs_file_read(lh_addr_of(file), buf + used, capacity - used);
            }
            else
            {
                /* Buffer full: the file fits only if nothing is left to read. */
                got = lh_os_fs_file_read(lh_addr_of(file), lh_addr_of(probe), 1U);
                ok = got == 0;
                break;
            }
            if (got < 0)
            {
                break;
            }
            if (got == 0)
            {
                ok = lh_bool_true;
                break;
            }
            used += lh_cast_static(lh_usize_t, got);
        }
    }
    lh_os_fs_file_deinit(lh_addr_of(file));
    *size = used;
    return ok;
}

static lh_bool_t
raspad_master_list_file_add_item(raspad_master_registry_t *registry, const lh_web_json_node_t *item)
{
    lh_str_view_t ip_key;
    lh_str_view_t port_key;
    lh_net_ip4_t ip;
    lh_ullong_t port_value;
    lh_net_ip4_socket_addr_t addr;

    if (!lh_web_json_node_has_kind(item, lh_web_json_kind_object))
    {
        return lh_bool_false;
    }
    lh_str_view_init(lh_addr_of(ip_key), "ip");
    lh_str_view_init(lh_addr_of(port_key), "port");
    const lh_web_json_node_t *ip_json = lh_web_json_node_get_member(item, ip_key);
    const lh_web_json_node_t *port_json = lh_web_json_node_get_member(item, port_key);
    if (lh_null_eq(ip_json) || !lh_web_json_node_has_kind(ip_json, lh_web_json_kind_string) ||
        lh_null_eq(port_json) || !lh_web_json_node_get_uint(port_json, lh_addr_of(port_value)))
    {
        return lh_bool_false;
    }
    const lh_str_view_t ip_text = lh_web_json_node_get_str(ip_json);
    if (!lh_net_ip4_parse(lh_str_view_get_begin(lh_addr_of(ip_text)),
                          lh_str_view_get_size(lh_addr_of(ip_text)), lh_addr_of(ip)))
    {
        return lh_bool_false;
    }
    if (port_value < 1U || port_value > LH_NET_PORT_MAX)
    {
        return lh_bool_false;
    }
    lh_net_ip4_socket_addr_init(&addr, lh_addr_of(ip), lh_cast_static(lh_net_port_t, port_value));
    return raspad_master_registry_add(registry, lh_addr_of(addr));
}

lh_bool_t
raspad_master_list_file_load(const lh_char_t *path, raspad_master_registry_t *registry)
{
    lh_usize_t size;
    lh_str_view_t view;
    lh_web_json_reader_t reader;
    lh_web_json_node_t root;
    raspad_master_registry_t next;
    lh_bool_t ok;

    lh_assert_runtime_ref(path);
    lh_assert_runtime_ref(registry);

    lh_memory_allocator_t *allocator = lh_runtime_allocator_get_mut();
    lh_char_t *text = (lh_char_t *)lh_memory_allocator_try_alloc(allocator, RASPAD_MASTER_LIST_FILE_MAX);
    if (lh_null_eq(text))
    {
        return lh_bool_false;
    }
    if (!raspad_master_list_file_read(path, text, RASPAD_MASTER_LIST_FILE_MAX, lh_addr_of(size)) ||
        size == 0U)
    {
        lh_memory_allocator_dealloc(allocator, text);
        return lh_bool_false;
    }
    lh_str_view_init_by_size(lh_addr_of(view), text, size);
    lh_web_json_node_init(lh_addr_of(root));
    lh_web_json_reader_init(lh_addr_of(reader), view);
    ok = lh_web_json_reader_read(lh_addr_of(reader), lh_addr_of(root));
    lh_memory_allocator_dealloc(allocator, text);

    /* A top-level array, or {"servers":[...]}. */
    const lh_web_json_node_t *servers = lh_null;
    if (ok)
    {
        if (lh_web_json_node_has_kind(lh_addr_of(root), lh_web_json_kind_array))
        {
            servers = lh_addr_of(root);
        }
        else
        {
            lh_str_view_t servers_key;
            lh_str_view_init(lh_addr_of(servers_key), "servers");
            servers = lh_web_json_node_get_member(lh_addr_of(root), servers_key);
            ok = lh_null_ne(servers) && lh_web_json_node_has_kind(servers, lh_web_json_kind_array);
        }
    }
    if (!ok)
    {
        lh_web_json_node_deinit(lh_addr_of(root));
        return lh_bool_false;
    }

    raspad_master_registry_init(lh_addr_of(next), registry->max_servers);
    for (const lh_web_json_node_t *item = lh_web_json_node_get_first(servers); lh_null_ne(item);
         item = lh_web_json_node_get_next(servers, item))
    {
        if (!raspad_master_list_file_add_item(lh_addr_of(next), item))
        {
            ok = lh_bool_false;
            break;
        }
    }
    if (ok)
    {
        raspad_master_registry_clear(registry);
        lh_usize_t count = raspad_master_registry_get_size(lh_addr_of(next));
        for (lh_usize_t i = 0; i < count; ++i)
        {
            lh_net_ip4_socket_addr_t addr;
            if (raspad_master_registry_get(lh_addr_of(next), i, lh_addr_of(addr)))
            {
                (void)raspad_master_registry_add(registry, lh_addr_of(addr));
            }
        }
    }
    raspad_master_registry_deinit(lh_addr_of(next));
    lh_web_json_node_deinit(lh_addr_of(root));
    return ok;
}
