#include <raspad/master/list_file.h>
#include <cJSON.h>
#include <lh/assert.h>
#include <lh/cast/static.h>
#include <lh/memory.h>
#include <lh/net/ip.h>
#include <lh/net/port.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/null.h>
#include <lh/str/ptr.h>
#include <lh/util/addr.h>
#include <stdio.h>
#include <sys/stat.h>

#if defined(_MSC_VER)
#    define RASPAD_STAT _stat
#    define RASPAD_STAT_T struct _stat
#else
#    define RASPAD_STAT stat
#    define RASPAD_STAT_T struct stat
#endif

#define RASPAD_MASTER_LIST_FILE_MAX 65536U

lh_s64_t
raspad_master_list_file_mtime(const lh_char_t *path)
{
    RASPAD_STAT_T info;

    if (lh_null_eq(path) || path[0] == '\0')
    {
        return 0;
    }
    if (RASPAD_STAT(path, &info) != 0)
    {
        return 0;
    }
    return lh_cast_static(lh_s64_t, info.st_mtime);
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
    addr = lh_net_ip4_socket_addr_make(lh_addr_of(ip), port);
    return raspad_master_registry_add(registry, lh_addr_of(addr));
}

lh_bool_t
raspad_master_list_file_load(const lh_char_t *path, raspad_master_registry_t *registry)
{
    FILE *file;
    lh_char_t *text;
    long size;
    size_t n;
    cJSON *root;
    cJSON *servers;
    cJSON *item;
    raspad_master_registry_t next;
    lh_bool_t ok;

    lh_assert_runtime_ref(path);
    lh_assert_runtime_ref(registry);

    file = fopen(path, "rb");
    if (file == lh_null)
    {
        return lh_bool_false;
    }
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return lh_bool_false;
    }
    size = ftell(file);
    if (size <= 0 || size > (long)RASPAD_MASTER_LIST_FILE_MAX)
    {
        fclose(file);
        return lh_bool_false;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return lh_bool_false;
    }
    text = (lh_char_t *)cJSON_malloc((size_t)size + 1U);
    if (lh_null_eq(text))
    {
        fclose(file);
        return lh_bool_false;
    }
    n = fread(text, 1, (size_t)size, file);
    fclose(file);
    text[n] = '\0';
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
