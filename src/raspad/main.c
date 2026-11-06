#include <lh/core/memory/allocator/initializer.h>
#include <lh/core/runtime/allocator.h>
#include <lh/core/runtime/logger.h>
#include <lh/core/runtime/terminate.h>
#include <lh/core/runtime/terminate/handler/initializer.h>
#include <lh/core/runtime/writer.h>
#include <lh/core/str/format/text.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lh/core/cast/static.h>
#include <lh/core/char.h>
#include <lh/core/datetime.h>
#include <lh/core/logger.h>
#include <lh/core/memory.h>
#include <lh/core/net/ip.h>
#include <lh/core/null.h>
#include <lh/core/numeric/types.h>
#include <lh/os/timestamp.h>
#include <lh/os/fs/path.h>
#include <lh/os/shared.h>
#include <lh/os/system/fs/path.h>
#include <lh/core/str/ptr.h>
#include <lh/os/net.h>
#include <lh/core/util/addr.h>
#include <raspad/master.h>
#include <stdarg.h>

static lh_ptr
host_alloc(lh_self_ptr self, lh_usize_t size)
{
    (void)self;
    return malloc(size);
}

static lh_void
host_dealloc(lh_self_ptr self, lh_ptr ptr)
{
    (void)self;
    free(ptr);
}

static lh_ptr
host_realloc(lh_self_ptr self, lh_ptr ptr, lh_usize_t old_size, lh_usize_t new_size)
{
    (void)self;
    (void)old_size;
    return realloc(ptr, new_size);
}

static lh_ssize_t
host_write(lh_self_ptr self, const lh_ptr buf, lh_usize_t size)
{
    (void)self;
    return (lh_ssize_t)fwrite(buf, 1u, size, stdout);
}

static lh_ssize_t
host_log(lh_self_ptr self, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    (void)self;
    (void)level;
    char line[1024];
    const lh_usize_t n = lh_str_ptr_format_text_v(line, sizeof(line) - 1u, fmt, args);
    fwrite(n != 0u ? line : fmt, 1u, n != 0u ? n : strlen(fmt), stderr);
    fputc('\n', stderr);
    fflush(stderr);
    return (lh_ssize_t)(n + 1u);
}

static lh_void
host_abort(lh_self_ptr self)
{
    (void)self;
    abort();
}

static void
host_runtime_init(void)
{
    const lh_memory_allocator_t allocator =
        lh_memory_allocator_initializer_with_realloc(host_alloc, host_dealloc, host_realloc);
    const lh_io_writer_t writer = {host_write, lh_null};
    const lh_logger_t logger = {lh_logger_level_flags_all, host_log, host_log, host_log,
                                host_log, host_log, host_log, host_log, host_log, lh_null};
    const lh_runtime_terminate_handler_t terminate =
        lh_runtime_terminate_handler_initializer(host_abort, lh_null);
    lh_memory_allocator_assign(lh_runtime_allocator_get_mut(), &allocator);
    lh_io_writer_assign(lh_runtime_writer_get_mut(), &writer);
    lh_logger_assign(lh_runtime_logger_get_mut(), &logger);
    lh_runtime_terminate_handler_assign(lh_runtime_terminate_get_mut(), &terminate);
}

static lh_str_cptr
raspad_master_log_level_name(lh_logger_level_t level)
{
    switch (level)
    {
    case lh_logger_level_emergency:
        return "emergency";
    case lh_logger_level_alert:
        return "alert";
    case lh_logger_level_critical:
        return "critical";
    case lh_logger_level_error:
        return "error";
    case lh_logger_level_warning:
        return "warning";
    case lh_logger_level_notice:
        return "notice";
    case lh_logger_level_info:
        return "info";
    case lh_logger_level_debug:
        return "debug";
    default:
        return "log";
    }
}

static lh_usize_t
raspad_master_log_stamp(lh_str_ptr out, lh_usize_t out_size)
{
    lh_datetime_t stamp;
    lh_timestamp_t timestamp;
    lh_usize_t n;

    if (!lh_os_timestamp_try_get_local_now(lh_addr_of(timestamp)))
    {
        return 0;
    }
    lh_timestamp_to_datetime(timestamp, lh_addr_of(stamp));
    n = lh_datetime_format(lh_addr_of(stamp), out, out_size);
    if (n == 0 || n >= out_size)
    {
        return 0;
    }
    out[n] = '\0';
    return n;
}

static lh_ssize_t
raspad_master_log_stderr(lh_ptr context, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    lh_char_t stamp[LH_DATETIME_TEXT_MAX + 1U];
    lh_char_t body[384];
    lh_char_t line[512];
    lh_usize_t body_n;
    lh_usize_t line_n;

    (void)context;
    body_n = lh_str_ptr_format_text_v(body, sizeof(body) - 1U, fmt, args);
    if (body_n == 0)
    {
        return 0;
    }
    body[body_n] = '\0';
    if (raspad_master_log_stamp(stamp, sizeof(stamp)) == 0)
    {
        stamp[0] = '?';
        stamp[1] = '\0';
    }
    line_n = lh_str_ptr_format_text(line, sizeof(line) - 1U, "%s %s %s", stamp,
                                    raspad_master_log_level_name(level), body);
    if (line_n == 0)
    {
        return 0;
    }
    line[line_n] = '\n';
    line[line_n + 1U] = '\0';
    fputs(line, stderr);
    return lh_cast_static(lh_ssize_t, line_n + 1U);
}

static void
raspad_master_fill_list_path(lh_char_t *out, lh_usize_t out_size)
{
    lh_os_shared_t executable;
    lh_fs_path_t dir;
    lh_fs_path_t name;
    lh_fs_path_t joined;
    lh_str_view_t name_view;
    lh_str_t rendered;
    lh_os_shared_init(&executable);
    lh_fs_path_init(&dir);
    lh_fs_path_init(&name);
    lh_fs_path_init(&joined);
    lh_str_view_init(&name_view, RASPAD_MASTER_LIST_PATH);
    lh_fs_path_set(&name, name_view, lh_os_system_fs_path_style_native());
    lh_memory_set(out, out_size, 0);
    if (lh_os_shared_bind_executable(&executable))
    {
        lh_fs_path_parent(&dir, lh_os_shared_get_path(&executable));
        lh_fs_path_join(&joined, &dir, &name);
        const lh_str_ptr path = lh_os_fs_path_to_cstr(&joined, &rendered);
        if (path != lh_null && lh_str_ptr_len(path) < out_size)
        {
            lh_memory_copy(out, out_size, path, lh_str_ptr_len(path) + 1U);
        }
        lh_str_deinit(&rendered);
    }
    if (out[0] == '\0')
    {
        lh_memory_copy(out, out_size, RASPAD_MASTER_LIST_PATH, sizeof(RASPAD_MASTER_LIST_PATH));
    }
    lh_fs_path_deinit(&joined);
    lh_fs_path_deinit(&name);
    lh_fs_path_deinit(&dir);
    lh_os_shared_deinit(&executable);
}

int
main(void)
{
    raspad_master_t master;
    raspad_master_config_t config;
    lh_logger_t logger;
    lh_char_t bind_text[LH_NET_IP4_TEXT_MAX + 1U];
    lh_usize_t bind_n;

    host_runtime_init();

    if (!lh_os_net_init())
    {
        return 1;
    }

    lh_logger_init(lh_addr_of(logger), lh_logger_level_flags_all, raspad_master_log_stderr, lh_null);
    config = raspad_master_config_make_default();
    raspad_master_fill_list_path(config.list_path, sizeof(config.list_path));
    raspad_master_init(lh_addr_of(master), lh_addr_of(config), lh_addr_of(logger));
    if (!raspad_master_bind(lh_addr_of(master)))
    {
        lh_logger_error(lh_addr_of(logger), "listen failed udp port %u",
                        lh_cast_static(lh_uint_t, config.udp_port));
        raspad_master_deinit(lh_addr_of(master));
        lh_os_net_deinit();
        return 1;
    }
    bind_n = lh_net_ip4_format(lh_addr_of(config.bind_ip), bind_text, sizeof(bind_text) - 1U);
    bind_text[bind_n] = '\0';
    lh_logger_info(lh_addr_of(logger), "listen udp %s:%u list file %s", bind_text,
                   lh_cast_static(lh_uint_t, config.udp_port), config.list_path);

    for (;;)
    {
        raspad_master_poll(lh_addr_of(master), 250ULL);
    }
}
