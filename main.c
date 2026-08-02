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


    loop(&ping);
    
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
gerer timeout => changer pour SELECT

test :
OK = ping 10.255.255.1 => wait for response et le ctrl c ne marche pas


test idea :
./myping unknown-domain-xyz
./myping google.com
./myping localhost
./myping 127.0.0.1
./myping 8.8.8.8

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
