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

static uint16_t calculate_checksum(uint16_t *data, int len)
{
    uint32_t sum = 0;

    for (int i = 0; i < len / 2; i++)
    {
        sum += data[i];
    }

    if (len % 2)
    {
        sum += ((uint8_t *)data)[len - 1];
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

static void send_packet(t_ping *ping)
{
    struct icmphdr packet = {
        .type = ICMP_ECHO,
        .code = 0,
        .checksum = 0,
        .un = {.echo = {.id = 0, .sequence = 0}}};

    packet.checksum = calculate_checksum((uint16_t *)&packet, sizeof(packet));
    printf("checksum: %u\n", packet.checksum);

    int result = sendto(ping->socket_fd, &packet, sizeof(packet), 0, (struct sockaddr *)ping->addr, sizeof(*ping->addr));
    if (result < 0)
    {
        perror("sendto failed");
    }
    else
    {
        // un packet echo devrai faire 8 octets
        printf("%sSent %d bytes to %s%s\n", GREEN, result, ping->dest, RESET);
    }
}

static void receive_packet(t_ping *ping)
{
    // receive packet
    char buffer[1024] = {0};
    printf("%sWaiting for a response...\n%s", YELLOW, RESET);

    int result = recvfrom(ping->socket_fd, buffer, 1024, 0, NULL, NULL);
    if (result < 0)
    {
        perror("recvfrom failed");
    }
    else
    {
        printf("%sReceived %d bytes from %s%s\n", GREEN, result, ping->dest, RESET);
    }

    // analyze packet

    // 1. sauter l'en-tête IP
    // struct iphdr *ip_hdr = (struct iphdr *)buffer;
    struct icmphdr *icmp = (struct icmphdr *)buffer;

    printf("type = %u\n", icmp->type);
    printf("code = %u\n", icmp->code);
    printf("id = %u\n", ntohs(icmp->un.echo.id));
    printf("seq = %u\n", ntohs(icmp->un.echo.sequence));

    printf("Packet size: %d bytes\n", result);

    for (int i = 0; i < result; i++)
    {
        printf("%02x ", (unsigned char)buffer[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");

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
        fprintf(stderr, "%sft_ping :    %s  - No address associated with hostname%s\n", RED, ping->dest, RESET);
        exit(EXIT_FAILURE);
    }

    // res->ai_addr contient maintenant la struct sockaddr avec l'IP résolue
    ping->addr = (struct sockaddr_in *)res->ai_addr;
    // free le res apres utilisation
    freeaddrinfo(res);

    printf("%sResolved IP address for %s - %s%s\n", GREEN, ping->dest, inet_ntoa(ping->addr->sin_addr), RESET);
    // Si tu veux recevoir le paquet IP complet il faut utiliser SOCK_RAW au lieu de SOCK_DGRAM
    // Avec SOCK_DGRAM + IPPROTO_ICMP, le noyau te fournit une interface "ICMP". Il construit une partie du paquet pour toi et, à la réception, il retire le header IP. C'est pour cela que tu reçois seulement les 8 octets du header ICMP
    // Le premier octet vaut 00 → c'est un ICMP Echo Reply (type = 0), donc c'est cohérent.

    ping->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ping->socket_fd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    send_packet(ping);

    receive_packet(ping);
}

void do_ping(t_ping *ping)
{
    signal(SIGINT, sigint_handler);

    while (g_running)
    {
        send_packet(ping);
        receive_packet(ping);
        sleep(1); // Simulate a ping delay
    }
}
