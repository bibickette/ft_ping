
#include "ft_ping.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

void receive_packet(int socket_fd, struct sockaddr_in *addr, ssize_t *packets_received, char *reverse_dns)
{
    // receive packet
    char buffer[1024] = {0};
    socklen_t addr_len = sizeof(*addr);

    int result = recvfrom(socket_fd, buffer, 1024, 0, (struct sockaddr *)addr, &addr_len);
    if (result < 0)
    {
        perror("recvfrom failed");
        return;
    }

    (*packets_received)++;
    struct iphdr *ip_hdr = (struct iphdr *)buffer;

    printf("%s%d bytes from %s (%s): ", GREEN, result - ip_hdr->ihl * 4, reverse_dns, inet_ntoa(addr->sin_addr));
    printf("icmp_seq=%u ", ntohs(((struct icmphdr *)(buffer + ip_hdr->ihl * 4))->un.echo.sequence));
    printf("ttl=%u%s\n",ip_hdr->ttl, RESET);


    // ip_hdr->ihl * 4 -> gives the size of the IP header in bytes, so we can use it to find the start of the ICMP header in the received packet.
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);

    if(icmp->type == ICMP_ECHOREPLY && icmp->un.echo.id == htons(getpid() & 0xffff)) {
        printf("Packet is Echo Reply\n");

    } else {
        printf("Received packet with UNexpected ID: %u\n",  ntohs(icmp->un.echo.id));
        printf("Received ICMP type %d code %d\n",  icmp->type, icmp->code);
    }


    printf("size: %d bytes\n\n", result);

    // for (int i = 0; i < result; i++)
    // {
    //     printf("%02x ", (unsigned char)buffer[i]);

    //     if ((i + 1) % 16 == 0)
    //         printf("\n");
    // }
    // printf("\n");
}
