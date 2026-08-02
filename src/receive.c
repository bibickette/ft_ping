
#include "ft_ping.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <errno.h>

void receive_packet(int socket_fd, struct sockaddr_in *addr, ssize_t *packets_received, char *reverse_dns, struct timeval *start_time, struct timeval *end_time)
{
    // receive packet
    char buffer[1024] = {0};
    socklen_t addr_len = sizeof(*addr);

    int result = recvfrom(socket_fd, buffer, 1024, 0, (struct sockaddr *)addr, &addr_len);
    if (result < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("Timeout occurred\n");
        }
        else
        {
            perror("recvfrom failed");
        }
        return;
    }

    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    // ip_hdr->ihl * 4 -> gives the size of the IP header in bytes, so we can use it to find the start of the ICMP header in the received packet.
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);

    if (icmp->type == ICMP_ECHOREPLY && icmp->un.echo.id == htons(getpid() & 0xffff))
    {
        printf("Packet is Echo Reply\n");
        printf("received id: %u, sequence number: %u\n", ntohs(icmp->un.echo.id), ntohs(icmp->un.echo.sequence));
        (*packets_received)++;
        gettimeofday(end_time, NULL);
        double rtt = (end_time->tv_sec - start_time->tv_sec) * 1000.0 + (end_time->tv_usec - start_time->tv_usec) / 1000.0;

        printf("%s%d bytes from %s (%s): ", GREEN, result - ip_hdr->ihl * 4, reverse_dns, inet_ntoa(addr->sin_addr));
        printf("icmp_seq=%u ", ntohs(((struct icmphdr *)(buffer + ip_hdr->ihl * 4))->un.echo.sequence));
        printf("ttl=%u ", ip_hdr->ttl);
        printf("rtt=%.2f ms%s\n", rtt, RESET);

        // printf("size: %d bytes\n\n", result);
    }
    else
    {
        if (icmp->type == ICMP_ECHO && icmp->un.echo.id == htons(getpid() & 0xffff))
        {
            printf("PACKET IS FROM MYSELF, ECHO REQUEST, RECEIVE AGAIN\n");
            receive_packet(socket_fd, addr, packets_received, reverse_dns, start_time, end_time);
            return;
        }
        else
        {
            printf("Received packet with UNexpected ID: %u\n", ntohs(icmp->un.echo.id));
            printf("Received ICMP type %d code %d\n", icmp->type, icmp->code);
        }
        return;
    }

    // for (int i = 0; i < result; i++)
    // {
    //     printf("%02x ", (unsigned char)buffer[i]);

    //     if ((i + 1) % 16 == 0)
    //         printf("\n");
    // }
    // printf("\n");
}
