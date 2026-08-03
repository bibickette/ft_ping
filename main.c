#include "ft_ping.h"


int main(int argc, char *argv[])
{

    t_ping ping;
    memset(&ping, 0, sizeof(t_ping));

    ping.size_payload = 56; // default payload size

    parse_opts(argc, argv, &ping);

    
    if (ping.mode & OPT_VERBOSE)
    {
        printf("Verbose mode enabled\n");
    }
    if (ping.mode & OPT_COUNT)
    {
        printf("Count mode enabled\n");
    }


    if(!loop(&ping)){
        if(ping.socket_fd >= 0){
            close(ping.socket_fd);
        }
        return 1;
    }
    
    if (ping.socket_fd >= 0){
        close(ping.socket_fd);
        printf("--- %s ping statistics ---\n", ping.dest);
        printf("%ld packets transmitted, %ld received, ", ping.packets_sent, ping.packets_received);
        printf("%ld%% packet loss\n", (ping.packets_sent - ping.packets_received) * 100 / ping.packets_sent);
    }

    printf("\n%sExiting ft_ping...%s\n", YELLOW, RESET);
    return 0;
}

/*
todo:
- RTT
- si le paquet a un late delay (10s au max pour aller retour) => perdu
    => le flag -W peut changer ce delay

- verbose :
        int pid = getpid() & 0xFFFF;
        printf(", id 0x%x = %i", pid, pid);
- dupplicata de packet : (peut y en avoir 1 ou +)
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
