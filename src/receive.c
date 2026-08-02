
#include "ft_ping.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <errno.h>

static bool verify_checksum(struct icmphdr *receive_data)
{
    uint16_t received_checksum = receive_data->checksum;
    receive_data->checksum = 0;
    uint16_t calculated_checksum = calculate_checksum((unsigned short *)receive_data, sizeof(struct icmphdr));
    receive_data->checksum = received_checksum;
    return received_checksum == calculated_checksum;
}

static bool is_addr_match(struct sockaddr_in *addr, struct sockaddr_in *recv_addr)
{
    return addr->sin_addr.s_addr == recv_addr->sin_addr.s_addr;
}

static bool is_echo_from_myself(struct icmphdr *receive_data)
{
    return ((receive_data->type == ICMP_ECHO && receive_data->un.echo.id == htons(getpid() & 0xffff)));
}

static bool is_my_pid(struct icmphdr *receive_data)
{
    return ((receive_data->type == ICMP_ECHOREPLY && receive_data->un.echo.id == htons(getpid() & 0xffff)));
}

/* changer la logique !!
si ce nest pas mon paquet => continue
*/
int receive_packet(int socket_fd, struct sockaddr_in *addr, ssize_t *packets_received, struct timeval *start_time, struct timeval *end_time)
{
    // receive packet
    char buffer[1024] = {0};
    struct sockaddr_in recv_addr = *addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    int result = recvfrom(socket_fd, buffer, 1024, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
    if (result < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("Timeout occurred\n");
            return TIMEOUT;
        }

        perror("recvfrom failed");
        return FAILED;
    }

    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    // ip_hdr->ihl * 4 -> gives the size of the IP header in bytes, so we can use it to find the start of the ICMP header in the received packet.
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
    // 1. verify type and id
    // 2. verify checksum
    // 3. verify sequence number
    // 4. verify source address
    if (!verify_checksum(icmp))
    {
        printf("Checksum verification failed\n");
        // return FAILED;
    }

    if (is_my_pid(icmp) && ntohs(icmp->un.echo.sequence) == *packets_received && is_addr_match(addr, &recv_addr))
    {

        (*packets_received)++;
        gettimeofday(end_time, NULL);
        double rtt = (end_time->tv_sec - start_time->tv_sec) * 1000.0 + (end_time->tv_usec - start_time->tv_usec) / 1000.0;

        printf("%s%d bytes from %s: ", GREEN, result - ip_hdr->ihl * 4, inet_ntoa(recv_addr.sin_addr));
        printf("icmp_seq=%u ", ntohs(((struct icmphdr *)(buffer + ip_hdr->ihl * 4))->un.echo.sequence));
        printf("ttl=%u ", ip_hdr->ttl);
        printf("rtt=%.2f ms%s\n\n", rtt, RESET);
    }
    else
    {
        if (is_echo_from_myself(icmp) || !is_my_pid(icmp))
        {
            // empeche de recevoir mon echo si ping localhost
            // si deux ping, regarde que cest bien pour moi, sinon continue a recevoir
            receive_packet(socket_fd, addr, packets_received, start_time, end_time);
            return SUCCESS;
        }
        else
        {
            // keep in a buffer the expected and received sequence numbers and IP addresses for debugging purposes
            char expected_ip[INET_ADDRSTRLEN];
            char received_ip[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &addr->sin_addr,
                      expected_ip, sizeof(expected_ip));

            inet_ntop(AF_INET, &recv_addr.sin_addr,
                      received_ip, sizeof(received_ip));
            // end keeping
            printf("Expected sequence number %zu ; received %u\n", *packets_received, ntohs(icmp->un.echo.sequence));
            printf("Expected source %s ; received %s\n", expected_ip, received_ip);
            printf("ID: %u\n", ntohs(icmp->un.echo.id));
            printf("Received ICMP type %d code %d\n", icmp->type, icmp->code);
        }
        return SUCCESS;
    }
    return SUCCESS;

    // for (int i = 0; i < result; i++)
    // {
    //     printf("%02x ", (unsigned char)buffer[i]);

    //     if ((i + 1) % 16 == 0)
    //         printf("\n");
    // }
    // printf("\n");
}
