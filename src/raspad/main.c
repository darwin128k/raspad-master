#include <lh/cast/static.h>
#include <lh/char.h>
#include <lh/datetime.h>
#include <lh/logger.h>
#include <lh/memory.h>
#include <lh/net/ip.h>
#include <lh/null.h>
#include <lh/numeric/types.h>
#include <lh/os/datetime.h>
#include <lh/os/fs/path.h>
#include <lh/os/net.h>
#include <lh/str/format/text.h>
#include <lh/util/addr.h>
#include <raspad/master.h>
#include <stdarg.h>
#include <stdio.h>

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
    lh_usize_t n;

    if (!lh_os_datetime_now(lh_addr_of(stamp)))
    {
        return 0;
    }
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
    lh_char_t dir[512];

    lh_memory_set(out, out_size, 0);
    if (!lh_os_fs_path_exe_dir(dir, sizeof(dir)))
    {
        lh_memory_copy(out, out_size, RASPAD_MASTER_LIST_PATH, sizeof(RASPAD_MASTER_LIST_PATH));
        return;
    }
    if (!lh_os_fs_path_join(out, out_size, dir, RASPAD_MASTER_LIST_PATH))
    {
        lh_memory_set(out, out_size, 0);
        lh_memory_copy(out, out_size, RASPAD_MASTER_LIST_PATH, sizeof(RASPAD_MASTER_LIST_PATH));
    }
}

int
main(void)
{
    raspad_master_t master;
    raspad_master_config_t config;
    lh_logger_t logger;
    lh_char_t bind_text[LH_NET_IP4_TEXT_MAX + 1U];
    lh_usize_t bind_n;

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
