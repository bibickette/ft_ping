#include "ft_ping.h"
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

unsigned short calculate_checksum(unsigned short *data, int count)
{
    unsigned short sum = 0;

    while(count > 1)
    {
        sum += *data++;
        count -= 2;
    }

    if (count > 0)
    {
        sum += *(unsigned char *)data;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
}

bool send_packet(int socket_fd, struct sockaddr_in *addr, ssize_t *sequence_number, char *dest, struct timeval *start_time)
{
    struct ping_packet
    {
        struct icmphdr icmp_hdr;
        char payload[56]; // 56 bytes of payload to make the total packet size 64 bytes
    };

    struct ping_packet packet = {
        .icmp_hdr = {
            .type = ICMP_ECHO,
            .code = 0,
            .checksum = 0,
            .un = {.echo = {.id =  htons(getpid() & 0xffff), .sequence = htons(*sequence_number )}}},
        .payload = {0}

    };

    packet.icmp_hdr.checksum = calculate_checksum((unsigned short *)&packet, sizeof(packet));
    gettimeofday(start_time, NULL);

    int result = sendto(socket_fd, &packet, sizeof(packet), 0, (struct sockaddr *)addr, sizeof(*addr));
    if (result < 0)
    {
        perror("sendto failed");
        return false;
    }
    else
    {
        // un packet echo devrai faire 8 octets
        (*sequence_number)++;
        // printf("send id : %u, sequence number: %u\n", ntohs(packet.icmp_hdr.un.echo.id), ntohs(packet.icmp_hdr.un.echo.sequence));
        printf("%sSent %d bytes to %s : payload = %zu ; icmphdr = %zu %s\n", YELLOW, result, dest, sizeof(packet.payload), sizeof(struct icmphdr), RESET);
        // printf("sent to ip: %s\n", inet_ntoa(addr->sin_addr));
        // printf("send id : %u, sequence number: %u\n", ntohs(packet.icmp_hdr.un.echo.id), ntohs(packet.icmp_hdr.un.echo.sequence));
    }
    return true;
    // printf("and checksum: %u\n", packet.icmp_hdr.checksum);

}