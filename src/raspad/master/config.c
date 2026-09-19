#include <raspad/master/config.h>
#include <lh/net/ip.h>

raspad_master_config_t
raspad_master_config_make_default(void)
{
    raspad_master_config_t config;

    config.bind_ip = lh_net_ip4_make(0, 0, 0, 0);
    config.udp_port = RASPAD_MASTER_UDP_PORT;
    config.seed_ip = lh_net_ip4_make(37, 230, 210, 218);
    config.seed_port = RASPAD_MASTER_SEED_PORT;
    config.flood_max_hits = 30U;
    config.flood_window_ms = 1000ULL;
    config.max_udp_bytes = 1400U;
    config.max_servers = 1024U;
    return config;
}
