/**
 * @file http.h
 * @brief Minimal HTTP/1.1 REST over ::lh_io_stream_t + cJSON.
 *
 * Routes:
 * - `GET /health` → `{"ok":true}`
 * - `GET /servers.json` → `{"count":N,"servers":[{"ip":"...","port":N},...]}`
 * - `POST /servers.json` body `{"ip":"...","port":N}` → register
 *
 * No TLS. One request per connection for the prototype.
 */

#ifndef RASPAD_MASTER_HTTP_H
#define RASPAD_MASTER_HTTP_H

#include <lh/bool.h>
#include <lh/char.h>
#include <lh/compiler/extern/c.h>
#include <lh/io/stream.h>
#include <lh/net/ip.h>
#include <lh/numeric/fixed/types.h>
#include <lh/ptr.h>
#include <lh/size.h>
#include <raspad/master/config.h>
#include <raspad/master/flood.h>
#include <raspad/master/registry.h>

/**
 * @typedef raspad_master_http_method_t
 * @brief Parsed request method.
 */
typedef lh_u8_t raspad_master_http_method_t;

#define raspad_master_http_method_unknown 0U
#define raspad_master_http_method_get 1U
#define raspad_master_http_method_post 2U

/**
 * @struct raspad_master_http_request
 * @typedef raspad_master_http_request_t
 * @brief Views into the caller's request buffer (not owned).
 */
struct raspad_master_http_request
{
    raspad_master_http_method_t method;
    const lh_char_t *path;
    lh_usize_t path_size;
    const lh_char_t *body;
    lh_usize_t body_size;
};
typedef struct raspad_master_http_request raspad_master_http_request_t;

LH_COMPILER_EXTERN_C_BEGIN

lh_bool_t
raspad_master_http_request_parse(const lh_ptr buf, lh_usize_t size,
                                 raspad_master_http_request_t *out);

/**
 * @brief Build a complete HTTP response for @p request into @p out.
 *
 * @return Bytes written, or 0 if @p out_size is too small.
 */
lh_usize_t
raspad_master_http_write_response(lh_ptr out, lh_usize_t out_size,
                                  raspad_master_registry_t *registry,
                                  const raspad_master_http_request_t *request);

/**
 * @brief Read one request from @p stream, apply flood, write one response.
 */
lh_bool_t
raspad_master_http_serve(lh_io_stream_t *stream, raspad_master_registry_t *registry,
                         raspad_master_flood_t *flood, const lh_net_ip4_t *peer,
                         lh_u64_t now_ms, const raspad_master_config_t *config);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_HTTP_H */
