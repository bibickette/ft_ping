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

bool send_packet(t_ping *ping, struct timeval *start_time)
{
    struct ping_packet
    {
        struct icmphdr icmp_hdr;
        char payload[PAYLOAD_SIZE]; // 56 bytes of payload to make the total packet size 64 bytes
    };

    struct ping_packet packet = {
        .icmp_hdr = {
            .type = ICMP_ECHO,
            .code = 0,
            .checksum = 0,
            .un = {.echo = {.id = htons(getpid() & 0xffff), .sequence = htons(ping->sequence_number)}}},
        .payload = {0}};
        
    gettimeofday(start_time, NULL);
    memmove((char *)&packet + sizeof(struct icmphdr), start_time, sizeof(*start_time)); // Copy the start time into the payload
    packet.icmp_hdr.checksum = calculate_checksum((unsigned short *)&packet, sizeof(packet));

    int result = sendto(ping->socket_fd, &packet, sizeof(packet), 0, (struct sockaddr *)&ping->addr, sizeof(ping->addr));
    if (result < 0)
    {
        perror("sendto failed");
        return false;
    }
    // printf("%d send bytes to %s, seq=%d, pid=%d\n", result, inet_ntoa(ping->addr.sin_addr), ping->sequence_number, ntohs(packet.icmp_hdr.un.echo.id));
    ping->sequence_received[ping->sequence_number % MAX_RECV_SEQ_SAVE] = false;
    ping->sequence_number++;
    ping->packets_sent++;
    return true;
}