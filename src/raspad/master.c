#include <raspad/master.h>
#include <lh/assert.h>
#include <lh/cast/static.h>
#include <lh/char.h>
#include <lh/compiler/os.h>
#include <lh/io/dgram.h>
#include <lh/io/stream.h>
#include <lh/net/ip.h>
#include <lh/null.h>
#include <lh/numeric/types.h>
#include <lh/os/clock.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>
#include <raspad/master/http.h>
#include <raspad/master/udp.h>

#if LH_COMPILER_OS == LH_COMPILER_OS_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    include <winsock2.h>
typedef SOCKET raspad_master_native_t;
#else
#    include <sys/select.h>
#    include <sys/time.h>
typedef int raspad_master_native_t;
#endif

void
raspad_master_init(raspad_master_t *self, const raspad_master_config_t *config, lh_logger_t *logger)
{
    lh_assert_runtime_ref(self);
    lh_assert_runtime_ref(config);

    self->config = *config;
    raspad_master_registry_init(lh_addr_of(self->registry), config->max_servers);
    raspad_master_flood_init(lh_addr_of(self->flood), config->flood_max_hits, config->flood_window_ms,
                             1024U);
    self->logger = logger;
    lh_os_net_socket_init(lh_addr_of(self->udp));
    lh_os_net_socket_init(lh_addr_of(self->http));
}

void
raspad_master_deinit(raspad_master_t *self)
{
    lh_assert_runtime_ref(self);
    lh_os_net_socket_close(lh_addr_of(self->udp));
    lh_os_net_socket_close(lh_addr_of(self->http));
    raspad_master_flood_deinit(lh_addr_of(self->flood));
    raspad_master_registry_deinit(lh_addr_of(self->registry));
    self->logger = lh_null;
}

lh_bool_t
raspad_master_bind(raspad_master_t *self)
{
    lh_net_ip4_socket_addr_t udp_addr;
    lh_net_ip4_socket_addr_t http_addr;

    lh_assert_runtime_ref(self);

    udp_addr = lh_net_ip4_socket_addr_make(lh_addr_of(self->config.bind_ip), self->config.udp_port);
    http_addr = lh_net_ip4_socket_addr_make(lh_addr_of(self->config.bind_ip), self->config.http_port);

    if (!lh_os_net_socket_open(lh_addr_of(self->udp), lh_os_net_socket_type_udp))
    {
        return lh_bool_false;
    }
    lh_os_net_socket_set_reuse_addr(lh_addr_of(self->udp), lh_bool_true);
    if (!lh_os_net_socket_bind(lh_addr_of(self->udp), lh_addr_of(udp_addr)))
    {
        return lh_bool_false;
    }

    if (!lh_os_net_socket_open(lh_addr_of(self->http), lh_os_net_socket_type_tcp))
    {
        return lh_bool_false;
    }
    lh_os_net_socket_set_reuse_addr(lh_addr_of(self->http), lh_bool_true);
    if (!lh_os_net_socket_bind(lh_addr_of(self->http), lh_addr_of(http_addr)))
    {
        return lh_bool_false;
    }
    if (!lh_os_net_socket_listen(lh_addr_of(self->http), 16))
    {
        return lh_bool_false;
    }
    return lh_bool_true;
}

static raspad_master_native_t
raspad_master_native(const lh_os_net_socket_t *sock)
{
    return lh_cast_static(raspad_master_native_t, lh_os_net_socket_get_handle(sock));
}

static void
raspad_master_on_udp(raspad_master_t *self)
{
    lh_uchar_t buf[2048];
    lh_net_ip4_socket_addr_t peer;
    lh_io_dgram_t dgram;
    lh_ssize_t n;

    dgram = lh_os_net_socket_get_dgram(lh_addr_of(self->udp));
    n = lh_io_dgram_recv(lh_addr_of(dgram), buf, sizeof(buf), lh_addr_of(peer));
    if (n <= 0)
    {
        return;
    }
    if (lh_null_ne(self->logger))
    {
        lh_logger_info(self->logger, "udp query bytes=%u", lh_cast_static(lh_uint_t, n));
    }
    raspad_master_udp_handle(lh_addr_of(dgram), lh_addr_of(self->registry), lh_addr_of(self->flood),
                             buf, lh_cast_static(lh_usize_t, n), lh_addr_of(peer), lh_os_clock_ms(),
                             lh_addr_of(self->config));
}

static void
raspad_master_on_http(raspad_master_t *self)
{
    lh_os_net_socket_t client;
    lh_net_ip4_socket_addr_t peer;
    lh_io_stream_t stream;
    lh_net_ip4_t peer_ip;

    lh_os_net_socket_init(lh_addr_of(client));
    if (!lh_os_net_socket_accept(lh_addr_of(self->http), lh_addr_of(client), lh_addr_of(peer)))
    {
        return;
    }
    peer_ip = lh_net_ip4_socket_addr_get_ip(lh_addr_of(peer));
    stream = lh_os_net_socket_get_stream(lh_addr_of(client));
    raspad_master_http_serve(lh_addr_of(stream), lh_addr_of(self->registry), lh_addr_of(self->flood),
                             lh_addr_of(peer_ip), lh_os_clock_ms(), lh_addr_of(self->config));
    lh_os_net_socket_close(lh_addr_of(client));
}

lh_bool_t
raspad_master_poll(raspad_master_t *self, lh_u64_t timeout_ms)
{
    fd_set read_set;
    struct timeval timeout;
    raspad_master_native_t udp_fd;
    raspad_master_native_t http_fd;
    raspad_master_native_t max_fd;
    lh_int_t n;

    lh_assert_runtime_ref(self);

    udp_fd = raspad_master_native(lh_addr_of(self->udp));
    http_fd = raspad_master_native(lh_addr_of(self->http));
    FD_ZERO(&read_set);
    FD_SET(udp_fd, &read_set);
    FD_SET(http_fd, &read_set);
    max_fd = udp_fd > http_fd ? udp_fd : http_fd;

    timeout.tv_sec = lh_cast_static(long, timeout_ms / 1000ULL);
    timeout.tv_usec = lh_cast_static(long, (timeout_ms % 1000ULL) * 1000ULL);

#if LH_COMPILER_OS == LH_COMPILER_OS_WINDOWS
    n = select(0, &read_set, 0, 0, lh_addr_of(timeout));
#else
    n = select(lh_cast_static(lh_int_t, max_fd + 1), &read_set, 0, 0, lh_addr_of(timeout));
#endif
    (void)max_fd;
    if (n <= 0)
    {
        return lh_cast_static(lh_bool_t, n == 0);
    }
    if (FD_ISSET(udp_fd, &read_set))
    {
        raspad_master_on_udp(self);
    }
    if (FD_ISSET(http_fd, &read_set))
    {
        raspad_master_on_http(self);
    }
    return lh_bool_true;
}
