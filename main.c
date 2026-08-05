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
        avg_time = ping->rtt.total / ping->packets_received;
    }

    double stddev = calculate_stddev(&ping->rtt, ping->packets_received, avg_time);
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

    ping.size_payload = 56;        // default payload size
    ping.time_select = LATE_REPLY; // default time select
    ping.count = -1;                // default count (no limit)

    parse_opts(argc, argv, &ping);

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
-W -> timeout select
-w -> timeout du programme
-c -> nombre de paquets a envoyer

todo:
- si le paquet a un late delay (10s au max pour aller retour) => perdu : a reregarder
    - W signifie de changer le temps dattente sur select mais ne dit pas que le paquet est perdu, cst si ya rien pdnt 1s


PRIORITY :
- add printf for verbose mode
- dupplicata de packet : (peut y en avoir 1 ou +)
-> la sequence est un uint16_t donc elle peut se reinitialiser a 0 apres 65535

➜  ft_ping git:(main) ✗ ./inetutils-2.0/ping/ping localhost
PING localhost (127.0.0.1): 56 data bytes
64 bytes from 127.0.0.1: icmp_seq=0 ttl=82 time=0.149 ms
64 bytes from 127.0.0.1: icmp_seq=0 ttl=82 time=0.197 ms (DUP!)
64 bytes from 127.0.0.1: icmp_seq=1 ttl=83 time=0.052 ms
64 bytes from 127.0.0.1: icmp_seq=1 ttl=83 time=0.111 ms (DUP!)
64 bytes from 127.0.0.1: icmp_seq=2 ttl=84 time=0.065 ms
^C--- localhost ping statistics ---
3 packets transmitted, 3 packets received, +2 duplicates, 0% packet loss
round-trip min/avg/max/stddev = 0.052/0.115/0.197/0.054 ms

CORRUPTION :
➜  ft_ping git:(main) ✗ sudo tc qdisc add dev lo root netem corrupt 50%
➜  ft_ping git:(main) ✗ ping -v localhost
PING localhost (127.0.0.1): 56 data bytes, id 0x2104 = 8452
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=2 ttl=64 time=0.142 ms
64 bytes from 127.0.0.1: icmp_seq=6 ttl=64 time=0.097 ms
64 bytes from 127.0.0.1: icmp_seq=7 ttl=64 time=0.066 ms
64 bytes from 127.0.0.1: icmp_seq=8 ttl=64 time=0.075 ms
64 bytes from 127.0.0.1: icmp_seq=10 ttl=64 time=0.069 ms
64 bytes from 127.0.0.1: icmp_seq=13 ttl=64 time=0.090 ms
64 bytes from 127.0.0.1: icmp_seq=14 ttl=64 time=0.068 ms
64 bytes from 127.0.0.1: icmp_seq=16 ttl=64 time=0.069 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=20 ttl=64 time=0.108 ms
64 bytes from 127.0.0.1: icmp_seq=24 ttl=64 time=0.086 ms
64 bytes from 127.0.0.1: icmp_seq=25 ttl=64 time=0.082 ms
64 bytes from 127.0.0.1: icmp_seq=26 ttl=64 time=0.087 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=28 ttl=64 time=0.091 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=29 ttl=64 time=0.184 ms
64 bytes from 127.0.0.1: icmp_seq=32 ttl=64 time=0.076 ms
64 bytes from 127.0.0.1: icmp_seq=35 ttl=64 time=0.084 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=37 ttl=64 time=0.128 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=38 ttl=64 time=0.207 ms
64 bytes from 127.0.0.1: icmp_seq=39 ttl=64 time=0.121 ms
64 bytes from 127.0.0.1: icmp_seq=40 ttl=64 time=0.159 ms
64 bytes from 127.0.0.1: icmp_seq=41 ttl=64 time=0.151 ms
64 bytes from 127.0.0.1: icmp_seq=42 ttl=64 time=0.054 ms

➜  ft_ping git:(main) ✗ ping -v localhost
PING localhost (127.0.0.1): 56 data bytes, id 0x2523 = 9507
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=2 ttl=64 time=0.123 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=9 ttl=64 time=-1099511627775999.875 ms
64 bytes from 127.0.0.1: icmp_seq=19 ttl=64 time=0.095 ms
checksum mismatch from 127.0.0.1
64 bytes from 127.0.0.1: icmp_seq=24 ttl=64 time=-1125899906842624000.000 ms
^C--- localhost ping statistics ---
35 packets transmitted, 4 packets received, 88% packet loss
round-trip min/avg/max/stddev = -1125899906842624000.000/-281749854617600000.000/0.123/487370466597508864.000 ms


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
