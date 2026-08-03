#include "ft_ping.h"
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

unsigned short calculate_checksum(unsigned short *data, int count)
{
    unsigned long sum = 0;

    while (count > 1)
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

    return (unsigned short)~sum;
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
            .un = {.echo = {.id = htons(getpid() & 0xffff), .sequence = htons(*sequence_number)}}},
        .payload = {0}
    };
    gettimeofday(start_time, NULL);
    memcpy((char *)&packet + sizeof(struct icmphdr), start_time, sizeof(*start_time)); // Copy the start time into the payload
    packet.icmp_hdr.checksum = calculate_checksum((unsigned short *)&packet, sizeof(packet));

    int result = sendto(socket_fd, &packet, sizeof(packet), 0, (struct sockaddr *)addr, sizeof(*addr));
    if (result < 0)
    {
        perror("sendto failed");
        return false;
    }
    (*sequence_number)++;
    printf("%sSent %d bytes to %s : payload = %zu ; icmphdr = %zu %s\n", YELLOW, result, dest, sizeof(packet.payload), sizeof(struct icmphdr), RESET);
    return true;
}