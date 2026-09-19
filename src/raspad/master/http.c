#include <raspad/master/http.h>
#include <cJSON.h>
#include <lh/assert.h>
#include <lh/cast/static.h>
#include <lh/char.h>
#include <lh/memory.h>
#include <lh/net/ip.h>
#include <lh/net/port.h>
#include <lh/net/socket/addr/ip4.h>
#include <lh/null.h>
#include <lh/numeric/types.h>
#include <lh/str/format/text.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <lh/util/str/ptr.h>

static const lh_char_t raspad_master_http_crlfcrlf[] = {'\r', '\n', '\r', '\n'};

static lh_bool_t
raspad_master_http_starts_with(const lh_char_t *buf, lh_usize_t size, lh_str_cptr prefix)
{
    lh_usize_t n;

    n = lh_str_ptr_len(prefix);
    if (size < n)
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t, lh_memory_compare(buf, n, prefix, n) == 0);
}

lh_bool_t
raspad_master_http_request_parse(const lh_ptr buf, lh_usize_t size,
                                 raspad_master_http_request_t *out)
{
    const lh_char_t *bytes;
    const lh_char_t *line_end;
    const lh_char_t *path;
    const lh_char_t *path_end;
    const lh_char_t *header_end;
    lh_usize_t i;

    lh_assert_runtime_ref(buf);
    lh_assert_runtime_ref(out);

    bytes = lh_ptr_ccast(lh_char_t, buf);
    out->method = raspad_master_http_method_unknown;
    out->path = lh_null;
    out->path_size = 0;
    out->body = lh_null;
    out->body_size = 0;

    line_end = bytes;
    for (i = 0; i < size; ++i)
    {
        if (bytes[i] == '\r')
        {
            line_end = bytes + i;
            break;
        }
    }
    if (line_end == bytes)
    {
        return lh_bool_false;
    }

    if (raspad_master_http_starts_with(bytes, (lh_usize_t)(line_end - bytes), "GET "))
    {
        out->method = raspad_master_http_method_get;
        path = bytes + 4;
    }
    else if (raspad_master_http_starts_with(bytes, (lh_usize_t)(line_end - bytes), "POST "))
    {
        out->method = raspad_master_http_method_post;
        path = bytes + 5;
    }
    else
    {
        return lh_bool_true;
    }

    path_end = path;
    while (path_end < line_end && *path_end != ' ')
    {
        ++path_end;
    }
    out->path = path;
    out->path_size = (lh_usize_t)(path_end - path);

    header_end = lh_ptr_ccast(lh_char_t, lh_memory_find(buf, size, raspad_master_http_crlfcrlf, 4));
    if (lh_null_ne(header_end))
    {
        const lh_char_t *body = header_end + 4;
        out->body = body;
        out->body_size = (lh_usize_t)(bytes + size - body);
    }
    return lh_bool_true;
}

static lh_bool_t
raspad_master_http_path_is(const raspad_master_http_request_t *request, lh_str_cptr path)
{
    lh_usize_t n = lh_str_ptr_len(path);
    if (request->path_size != n)
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t, lh_memory_compare(request->path, n, path, n) == 0);
}

static lh_usize_t
raspad_master_http_write_status(lh_char_t *out, lh_usize_t out_size, lh_uint_t status,
                                lh_str_cptr reason, lh_str_cptr body)
{
    lh_char_t header[160];
    lh_usize_t body_size;
    lh_usize_t header_size;
    lh_usize_t total;

    body_size = lh_str_ptr_len(body);
    header_size = lh_str_ptr_format_text(header, sizeof(header),
                                         "HTTP/1.1 %u %s\r\nContent-Type: application/json\r\n"
                                         "Content-Length: %u\r\nConnection: close\r\n\r\n",
                                         status, reason, lh_cast_static(lh_uint_t, body_size));
    if (header_size == 0)
    {
        return 0;
    }
    total = header_size + body_size;
    if (out_size < total)
    {
        return 0;
    }
    lh_memory_copy(out, out_size, header, header_size);
    lh_memory_copy(out + header_size, out_size - header_size, body, body_size);
    return total;
}

static lh_str_ptr
raspad_master_http_servers_json(raspad_master_registry_t *registry)
{
    cJSON *root;
    cJSON *list;
    lh_usize_t i;
    lh_usize_t n;
    lh_str_ptr printed;

    root = cJSON_CreateObject();
    list = cJSON_AddArrayToObject(root, "servers");
    n = raspad_master_registry_get_size(registry);
    cJSON_AddNumberToObject(root, "count", lh_cast_static(double, n));
    for (i = 0; i < n; ++i)
    {
        lh_net_ip4_socket_addr_t addr;
        lh_net_ip4_t ip;
        lh_char_t ip_text[LH_NET_IP4_TEXT_MAX + 1U];
        cJSON *item;

        if (!raspad_master_registry_get(registry, i, lh_addr_of(addr)))
        {
            continue;
        }
        ip = lh_net_ip4_socket_addr_get_ip(lh_addr_of(addr));
        ip_text[lh_net_ip4_format(lh_addr_of(ip), ip_text, LH_NET_IP4_TEXT_MAX)] = '\0';
        item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ip", ip_text);
        cJSON_AddNumberToObject(item, "port", lh_net_ip4_socket_addr_get_port(lh_addr_of(addr)));
        cJSON_AddItemToArray(list, item);
    }
    printed = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return printed;
}

static lh_bool_t
raspad_master_http_add_from_json(raspad_master_registry_t *registry, const lh_char_t *body,
                                 lh_usize_t body_size)
{
    cJSON *root;
    cJSON *ip_json;
    cJSON *port_json;
    lh_net_ip4_t ip;
    lh_net_port_t port;
    lh_net_ip4_socket_addr_t addr;
    lh_bool_t ok;
    lh_char_t *copy;

    if (body_size == 0)
    {
        return lh_bool_false;
    }
    copy = cJSON_malloc(body_size + 1U);
    if (lh_null_eq(copy))
    {
        return lh_bool_false;
    }
    lh_memory_copy(copy, body_size + 1U, body, body_size);
    copy[body_size] = '\0';
    root = cJSON_Parse(copy);
    cJSON_free(copy);
    if (lh_null_eq(root))
    {
        return lh_bool_false;
    }
    ip_json = cJSON_GetObjectItemCaseSensitive(root, "ip");
    port_json = cJSON_GetObjectItemCaseSensitive(root, "port");
    ok = lh_bool_false;
    if (cJSON_IsString(ip_json) && cJSON_IsNumber(port_json))
    {
        if (lh_net_ip4_parse(ip_json->valuestring, lh_str_ptr_len(ip_json->valuestring),
                             lh_addr_of(ip)))
        {
            double port_value = port_json->valuedouble;
            if (port_value >= 0 && port_value <= LH_NET_PORT_MAX)
            {
                port = lh_cast_static(lh_net_port_t, port_value);
                addr = lh_net_ip4_socket_addr_make(lh_addr_of(ip), port);
                ok = raspad_master_registry_add(registry, lh_addr_of(addr));
            }
        }
    }
    cJSON_Delete(root);
    return ok;
}

lh_usize_t
raspad_master_http_write_response(lh_ptr out, lh_usize_t out_size,
                                  raspad_master_registry_t *registry,
                                  const raspad_master_http_request_t *request)
{
    lh_char_t *bytes;

    lh_assert_runtime_ref(out);
    lh_assert_runtime_ref(registry);
    lh_assert_runtime_ref(request);
    bytes = lh_ptr_cast(lh_char_t, out);

    if (request->method == raspad_master_http_method_unknown)
    {
        return raspad_master_http_write_status(bytes, out_size, 405U, "Method Not Allowed",
                                               "{\"error\":\"method\"}");
    }
    if (request->method == raspad_master_http_method_get &&
        raspad_master_http_path_is(request, "/health"))
    {
        return raspad_master_http_write_status(bytes, out_size, 200U, "OK", "{\"ok\":true}");
    }
    if (request->method == raspad_master_http_method_get &&
        raspad_master_http_path_is(request, "/servers.json"))
    {
        lh_str_ptr json = raspad_master_http_servers_json(registry);
        lh_usize_t n;

        if (lh_null_eq(json))
        {
            return raspad_master_http_write_status(bytes, out_size, 500U, "Internal Server Error",
                                                   "{\"error\":\"json\"}");
        }
        n = raspad_master_http_write_status(bytes, out_size, 200U, "OK", json);
        cJSON_free(json);
        return n;
    }
    if (request->method == raspad_master_http_method_post &&
        raspad_master_http_path_is(request, "/servers.json"))
    {
        if (!raspad_master_http_add_from_json(registry, request->body, request->body_size))
        {
            return raspad_master_http_write_status(bytes, out_size, 400U, "Bad Request",
                                                   "{\"error\":\"body\"}");
        }
        return raspad_master_http_write_status(bytes, out_size, 200U, "OK", "{\"ok\":true}");
    }
    return raspad_master_http_write_status(bytes, out_size, 404U, "Not Found",
                                           "{\"error\":\"path\"}");
}

lh_bool_t
raspad_master_http_serve(lh_io_stream_t *stream, raspad_master_registry_t *registry,
                         raspad_master_flood_t *flood, const lh_net_ip4_t *peer, lh_u64_t now_ms,
                         const raspad_master_config_t *config)
{
    lh_char_t req[4096];
    lh_char_t res[8192];
    lh_ssize_t n;
    raspad_master_http_request_t request;
    lh_usize_t written;

    lh_assert_runtime_ref(stream);
    lh_assert_runtime_ref(registry);
    lh_assert_runtime_ref(flood);
    lh_assert_runtime_ref(peer);
    lh_assert_runtime_ref(config);

    n = lh_io_stream_read(stream, req, config->max_http_bytes < sizeof(req) ? config->max_http_bytes
                                                                            : sizeof(req));
    if (n <= 0)
    {
        return lh_bool_false;
    }
    if (!raspad_master_flood_allow(flood, peer, now_ms))
    {
        written = raspad_master_http_write_status(res, sizeof(res), 429U, "Too Many Requests",
                                                  "{\"error\":\"flood\"}");
        if (written != 0)
        {
            lh_io_stream_write(stream, res, written);
        }
        return lh_bool_false;
    }
    if (!raspad_master_http_request_parse(req, lh_cast_static(lh_usize_t, n), lh_addr_of(request)))
    {
        return lh_bool_false;
    }
    written = raspad_master_http_write_response(res, sizeof(res), registry, lh_addr_of(request));
    if (written == 0)
    {
        return lh_bool_false;
    }
    return lh_cast_static(lh_bool_t, lh_io_stream_write(stream, res, written) ==
                                         lh_cast_static(lh_ssize_t, written));
}
