#include "ft_ping.h"

int main(int argc, char *argv[])
{

    t_ping ping;
    memset(&ping, 0, sizeof(t_ping));

    ping.linger_ms = LINGER_TIME * MILLISEC_PRECISION; // default linger time in milliseconds
    ping.interval_ms = TIMEOUT_SEC * MILLISEC_PRECISION; // default interval in milliseconds
    
    parse_opts(argc, argv, &ping);
    
    ping.pid = getpid() & 0xFFFF;

    resolve_dest(&ping);
    int ret = 0;
    ret = loop(&ping);

    close(ping.socket_fd);

    if (ret)
    {
        print_stats(&ping);
    }
    else
    {
        return 1;
    }
    return 0;
}

/*
todo:
- readme
- gerer les cas qui ne sont pas echo reply
- clean le code
- retester

*/
