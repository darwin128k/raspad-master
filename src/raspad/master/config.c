#include <raspad/master/config.h>
#include <lh/memory.h>
#include <lh/net/ip.h>

raspad_master_config_t
raspad_master_config_make_default(void)
{
    raspad_master_config_t config;
    static const lh_char_t path[] = RASPAD_MASTER_LIST_PATH;

    config.bind_ip = lh_net_ip4_make(0, 0, 0, 0);
    config.udp_port = RASPAD_MASTER_UDP_PORT;
    lh_memory_set(config.list_path, sizeof(config.list_path), 0);
    lh_memory_copy(config.list_path, sizeof(config.list_path), path, sizeof(path));
    config.flood_max_hits = 30U;
    config.flood_window_ms = 1000ULL;
    config.max_udp_bytes = 1400U;
    config.max_servers = 1024U;
    return config;
}
