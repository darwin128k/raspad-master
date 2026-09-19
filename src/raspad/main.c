#include <lh/cast/static.h>
#include <lh/char.h>
#include <lh/logger.h>
#include <lh/memory.h>
#include <lh/null.h>
#include <lh/numeric/types.h>
#include <lh/os/fs/path.h>
#include <lh/os/net.h>
#include <lh/str/format/text.h>
#include <lh/util/addr.h>
#include <raspad/master.h>
#include <stdarg.h>
#include <stdio.h>

static lh_ssize_t
raspad_master_log_stderr(lh_ptr context, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    lh_char_t buf[256];
    lh_usize_t n;

    (void)context;
    (void)level;
    n = lh_str_ptr_format_text_v(buf, sizeof(buf) - 1U, fmt, args);
    if (n == 0)
    {
        return 0;
    }
    buf[n] = '\n';
    buf[n + 1U] = '\0';
    fputs(buf, stderr);
    return lh_cast_static(lh_ssize_t, n + 1U);
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
        raspad_master_deinit(lh_addr_of(master));
        lh_os_net_deinit();
        return 1;
    }
    lh_logger_info(lh_addr_of(logger), "udp %u list %s", lh_cast_static(lh_uint_t, config.udp_port),
                   config.list_path);

    for (;;)
    {
        raspad_master_poll(lh_addr_of(master), 250ULL);
    }
}
