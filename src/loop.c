#include "ft_ping.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

void resolve_destination(t_ping *ping)
{

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = 0;

    int status = getaddrinfo(ping->dest, NULL, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "%sping: %s: %s%s\n", RED, ping->dest, gai_strerror(status), RESET);
        exit(EXIT_FAILURE);
    }

    // res->ai_addr contient maintenant la struct sockaddr avec l'IP résolue
    // free le res apres utilisation
    memcpy(&ping->addr, res->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(res);

    // Translate a socket address to a location and service name.
    // DNS Domain Name System : classique ( name -> IP ) mais aussi reverse ( IP -> name )
    if (getnameinfo((struct sockaddr *)&ping->addr, sizeof(ping->addr), ping->reverse_dns, sizeof(ping->reverse_dns), NULL, 0, 0) != 0)
    {
        perror("getnameinfo failed");
        exit(EXIT_FAILURE);
    }

    printf("%sPING %s (%s) %lu(%lu) bytes of data.%s\n", GREEN, ping->dest, inet_ntoa(ping->addr.sin_addr), ping->size_payload, ping->size_payload + sizeof(struct iphdr) + sizeof(struct icmphdr), RESET);
    // Si tu veux recevoir le paquet IP complet il faut utiliser SOCK_RAW au lieu de SOCK_DGRAM
    // Avec SOCK_DGRAM + IPPROTO_ICMP, le noyau te fournit une interface "ICMP". Il construit une partie du paquet pour toi et, à la réception, il retire le header IP. C'est pour cela que tu reçois seulement les 8 octets du header ICMP
    // Le premier octet vaut 00 → c'est un ICMP Echo Reply (type = 0), donc c'est cohérent.
    // du coup pour le socket raw il faut utiliser sudo
    ping->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ping->socket_fd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // ajouter setsockopt pour le timeout de recvfrom instead of blocking indefinitely
}

void loop(t_ping *ping)
{
    signal(SIGINT, sigint_handler);

    while (g_running)
    {
        send_packet(ping->socket_fd, &ping->addr, &ping->packets_sent, ping->dest);
        receive_packet(ping->socket_fd, &ping->addr, &ping->packets_received, ping->reverse_dns);
        sleep(1); // Simulate a ping delay
    }
}
