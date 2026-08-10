#include "ft_ping.h"
#include <math.h>

double calculate_stddev(t_rtt *rtt, ssize_t packets_received, double avg_time)
{
    if (packets_received == 0)
    {
        return 0.0;
    }
    double variance = (rtt->total_squared / packets_received) - (avg_time * avg_time);
    if (variance < 0.0)
    {
        variance = 0.0;
    }
    return sqrt(variance);
}

void print_stats(t_ping *ping)
{
    printf("--- %s ping statistics ---\n", ping->dest);
    printf("%ld packets transmitted, %ld received, ", ping->packets_sent, ping->packets_received);
    if (ping->duplicates > 0)
    {
        printf("+%d duplicates, ", ping->duplicates);
    }
    printf("%ld%% packet loss\n", (ping->packets_sent - ping->packets_received) * 100 / ping->packets_sent);

    double avg_time = 0.0;
    if (ping->packets_received != 0)
    {
        avg_time = ping->rtt.total / (ping->packets_received + ping->duplicates);
    }

    double stddev = calculate_stddev(&ping->rtt, ping->packets_received + ping->duplicates, avg_time);
    printf("round-trip min/avg/max/stddev = ");
    printf("%.3f/", ping->rtt.min);
    printf("%.3f/", avg_time);
    printf("%.3f/", ping->rtt.max);
    printf("%.3f ms\n", stddev);
}

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


-w ok
-W ok
-c ok

test idea :
OK : ./ft_ping unknown-domain-xyz
OK : ./ft_ping google.com
OK : ./ft_ping localhost
OK : ./ft_ping 127.0.0.1
OK : ./ft_ping 8.8.8.8
OK : ./ft_ping 10.255.255.1 => la reponse ne vient jamais mais il envoie des paquets
OK : sans internet

OK : avec un ping delay le time est OK



=== Les edge cases auxquels je penserais

Pour un projet ping, fais-toi une checklist.

Réseau
✅ réponse normale
❌ timeout
❌ 100% packet loss
⚠️ packet loss partiel
⚠️ RTT très élevé
⚠️ RTT variable
⚠️ réponse ICMP inattendue
⚠️ paquet corrompu / checksum incorrect
⚠️ réponse qui correspond à un autre ping

Et encore :

ICMP Destination Unreachable

→ tu as reçu une réponse ICMP, mais ce n'est pas un Echo Reply indiquant que la destination a répondu normalement.






*/
