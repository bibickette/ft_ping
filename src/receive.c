
#include "ft_ping.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

// si on recalcule le checksun et quon tombe sur 0 c'est que le checksum est valide
static bool verify_checksum(struct icmphdr *receive_data, int icmp_len)
{
    uint16_t calculated_checksum = calculate_checksum((unsigned short *)receive_data, icmp_len);
    return calculated_checksum == 0;
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

static bool is_socket_dgram(t_ping *ping)
{
    return ping->socket_dgram;
}

void fill_rtt(t_rtt *rtt, double time_packet)
{
    if (rtt->min == 0 || time_packet < rtt->min)
    {
        rtt->min = time_packet;
    }
    if (time_packet > rtt->max)
    {
        rtt->max = time_packet;
    }
    rtt->total += time_packet;
    rtt->total_squared += time_packet * time_packet;
}

enum packet_error
{
    ERR_ECHO_FROM_MYSELF = 1,
    ERR_NOT_MY_PID = 2,
    ERR_ADDR_MISMATCH = 3,
    ERR_CHECKSUM_INVALID = 4,
};

bool is_duplicate(t_ping *ping, uint16_t sequence_number)
{
    return ping->sequence_received[sequence_number % MAX_RECV_SEQ_SAVE];
}

bool receive_packet(t_ping *ping, struct timeval *end_time)
{
    char buffer[RECV_BUFFER_SIZE] = {0};
    /* garder dans un autre sockaddr_in car il peut overwrite mon addr source sinon */
    struct sockaddr_in recv_addr = ping->addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    int result = recvfrom(ping->socket_fd, buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
    if (result < 0)
    {
        perror("recvfrom failed");
        return false;
    }

    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    // ip_hdr->ihl * 4 -> gives the size of the IP header in bytes, so we can use it to find the start of the ICMP header in the received packet.
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
    // Length of the ICMP packet icmp header + payload
    int icmp_len = result - ip_hdr->ihl * 4;
    int error = 0;
    // 1. verify type and id
    // 2. verify source address
    // 3. verify checksum
    if (is_echo_from_myself(icmp))
    {
        error = ERR_ECHO_FROM_MYSELF;
    }
    else if (!is_my_pid(icmp) && !is_socket_dgram(ping))
    {
        error = ERR_NOT_MY_PID;
    }
    else if (!is_addr_match(&ping->addr, &recv_addr))
    {
        error = ERR_ADDR_MISMATCH;
    }
    else if (!verify_checksum(icmp, icmp_len))
    {
        error = ERR_CHECKSUM_INVALID;
    }

    if (error)
    {
        if (!(ping->mode & OPT_VERBOSE) && error != ERR_CHECKSUM_INVALID)
        {
            return true;
        }
        else if (ping->mode & OPT_VERBOSE && error != ERR_CHECKSUM_INVALID)
        {
            switch (error)
            {
            case ERR_ECHO_FROM_MYSELF:
                printf("%secho request from myself, icmp_seq : %u%s\n", YELLOW, ntohs(icmp->un.echo.sequence), RESET);
                return true;
            case ERR_NOT_MY_PID:
                printf("%secho reply from unexpected pid : %d - src pid : %d%s\n", YELLOW, ntohs((uint16_t)icmp->un.echo.id), getpid() & 0xffff, RESET);
                return true;
            case ERR_ADDR_MISMATCH:
                // keep in a buffer the expected and received sequence numbers and IP addresses for debugging purposes
                char expected_ip[INET_ADDRSTRLEN];
                char received_ip[INET_ADDRSTRLEN];

                inet_ntop(AF_INET, &ping->addr.sin_addr,
                          expected_ip, sizeof(expected_ip));

                inet_ntop(AF_INET, &recv_addr.sin_addr,
                          received_ip, sizeof(received_ip));
                printf("%secho reply from unexpected ip : %s - src ip : %s%s\n", YELLOW, received_ip, expected_ip, RESET);
                return true;
            }
        }
        else if (error == ERR_CHECKSUM_INVALID)
        {
            printf("checksum mismatch from %s\n", inet_ntoa(recv_addr.sin_addr));
            return true;
        }
    }

    char payload[56] = {0};
    // mettre le time de l'envoi dans le payload et le recupere ici pour calculer le rtt
    memmove(payload, buffer + ip_hdr->ihl * 4 + sizeof(struct icmphdr), sizeof(payload));
    struct timeval *sent_time = (struct timeval *)payload;

    gettimeofday(end_time, NULL);
    double time_packet = (end_time->tv_sec - sent_time->tv_sec) * 1000.0 + (end_time->tv_usec - sent_time->tv_usec) / 1000.0;

    printf("%d bytes from %s: ", result - ip_hdr->ihl * 4, inet_ntoa(recv_addr.sin_addr));
    printf("icmp_seq=%u ", ntohs(icmp->un.echo.sequence));
    printf("ttl=%u ", ip_hdr->ttl);
    printf("rtt=%.3f ms", time_packet);
    if (is_duplicate(ping, ntohs(icmp->un.echo.sequence)))
    {
        printf(" (DUP!)");
        ping->duplicates++;
    }
    else
    {
        ping->packets_received++;
        ping->sequence_received[ntohs(icmp->un.echo.sequence) % MAX_RECV_SEQ_SAVE] = true;
    }
    fill_rtt(&ping->rtt, time_packet);
    printf("\n");
    return true;
}
