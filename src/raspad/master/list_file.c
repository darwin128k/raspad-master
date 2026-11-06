#include <raspad/master/list_file.h>
#include <cJSON.h>
#include <lh/core/assert.h>
#include <lh/core/cast/static.h>
#include <lh/core/memory.h>
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
raspad_master_list_file_add_item(raspad_master_registry_t *registry, const cJSON *item)
{
    const cJSON *ip_json;
    const cJSON *port_json;
    lh_net_ip4_t ip;
    lh_net_port_t port;
    lh_net_ip4_socket_addr_t addr;
    double port_value;

    if (!cJSON_IsObject(item))
    {
        return lh_bool_false;
    }
    ip_json = cJSON_GetObjectItemCaseSensitive(item, "ip");
    port_json = cJSON_GetObjectItemCaseSensitive(item, "port");
    if (!cJSON_IsString(ip_json) || !cJSON_IsNumber(port_json))
    {
        return lh_bool_false;
    }
    if (!lh_net_ip4_parse(ip_json->valuestring, lh_str_ptr_len(ip_json->valuestring),
                          lh_addr_of(ip)))
    {
        return lh_bool_false;
    }
    port_value = port_json->valuedouble;
    if (port_value < 1 || port_value > LH_NET_PORT_MAX)
    {
        return lh_bool_false;
    }
    port = lh_cast_static(lh_net_port_t, port_value);
    lh_net_ip4_socket_addr_init(&addr, lh_addr_of(ip), port);
    return raspad_master_registry_add(registry, lh_addr_of(addr));
}

lh_bool_t
raspad_master_list_file_load(const lh_char_t *path, raspad_master_registry_t *registry)
{
    lh_char_t *text;
    lh_usize_t size;
    cJSON *root;
    cJSON *servers;
    cJSON *item;
    raspad_master_registry_t next;
    lh_bool_t ok;

    lh_assert_runtime_ref(path);
    lh_assert_runtime_ref(registry);

    text = (lh_char_t *)cJSON_malloc(RASPAD_MASTER_LIST_FILE_MAX + 1U);
    if (lh_null_eq(text))
    {
        return lh_bool_false;
    }
    if (!raspad_master_list_file_read(path, text, RASPAD_MASTER_LIST_FILE_MAX, lh_addr_of(size)) ||
        size == 0U)
    {
        cJSON_free(text);
        return lh_bool_false;
    }
    text[size] = '\0';
    root = cJSON_Parse(text);
    cJSON_free(text);
    if (lh_null_eq(root))
    {
        return lh_bool_false;
    }
    if (cJSON_IsArray(root))
    {
        servers = root;
    }
    else
    {
        servers = cJSON_GetObjectItemCaseSensitive(root, "servers");
        if (!cJSON_IsArray(servers))
        {
            cJSON_Delete(root);
            return lh_bool_false;
        }
    }

    raspad_master_registry_init(lh_addr_of(next), registry->max_servers);
    ok = lh_bool_true;
    cJSON_ArrayForEach(item, servers)
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
        {
            lh_usize_t i;
            lh_usize_t count = raspad_master_registry_get_size(lh_addr_of(next));
            for (i = 0; i < count; ++i)
            {
                lh_net_ip4_socket_addr_t addr;
                if (raspad_master_registry_get(lh_addr_of(next), i, lh_addr_of(addr)))
                {
                    (void)raspad_master_registry_add(registry, lh_addr_of(addr));
                }
            }
        }
    }
    raspad_master_registry_deinit(lh_addr_of(next));
    cJSON_Delete(root);
    return ok;
}
