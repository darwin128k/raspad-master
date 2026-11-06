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
#include <gtest/gtest.h>

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

int
main(int argc, char **argv)
{
    host_runtime_init();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
