
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

static bool is_ip_match(uint32_t ip1, uint32_t ip2)
{
    return ip1 == ip2;
}

static bool is_echo_from_myself(struct icmphdr *receive_data, uint16_t pid)
{
    return ((receive_data->type == ICMP_ECHO && receive_data->un.echo.id == htons(pid)));
}

static bool is_for_my_pid(struct icmphdr *receive_data, uint16_t pid)
{
    return (receive_data->un.echo.id == htons(pid));
}

bool is_duplicate(t_ping *ping, uint16_t sequence_number)
{
    return ping->sequence_received[sequence_number % MAX_RECV_SEQ_SAVE];
}

static void fill_rtt(t_rtt *rtt, double time_packet)
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
    ERR_NOT_ECHOREPLY = 5
};

static void handle_error(int error, int mode, uint16_t pid, struct icmphdr *icmp, struct sockaddr_in from_addr, struct sockaddr_in recv_addr, struct iphdr *ip_hdr)
{
    if (!(mode & OPT_VERBOSE) && error != ERR_CHECKSUM_INVALID)
    {
        // if no verbose mode, no need to print anything, just return
        return;
    }
    else if (mode & OPT_VERBOSE && error != ERR_CHECKSUM_INVALID)
    {
        switch (error)
        {
        case ERR_ECHO_FROM_MYSELF:
            printf("%secho request from myself, icmp_seq : %u%s\n", YELLOW, ntohs(icmp->un.echo.sequence), RESET);
            return;
        case ERR_NOT_ECHOREPLY:
            // here handle different type
            printf("%snot an echo reply, icmp type : %d%s\n", YELLOW, icmp->type, RESET);
                        char expected_ip_addr[INET_ADDRSTRLEN];
            char received_ip_addr[INET_ADDRSTRLEN];
            char received_ip_packet[INET_ADDRSTRLEN];
            char expected_ip_packet[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from_addr.sin_addr,
                      expected_ip_addr, sizeof(expected_ip_addr));
            inet_ntop(AF_INET, &recv_addr.sin_addr,
                      received_ip_addr, sizeof(received_ip_addr));

            inet_ntop(AF_INET, &ip_hdr->saddr,
                      received_ip_packet, sizeof(received_ip_packet));
            inet_ntop(AF_INET, &ip_hdr->daddr,
                      expected_ip_packet, sizeof(expected_ip_packet));
            printf("%secho reply from unexpected ip :\n    - data in addr src : %s - dst ip : %s\n    - data in packet dst : %s - src : %s%s\n", YELLOW, received_ip_addr, expected_ip_addr, received_ip_packet, expected_ip_packet, RESET);
            
            return;
        case ERR_NOT_MY_PID:
            printf("%sping from unexpected pid : %d - src pid : %d%s\n", YELLOW, ntohs((uint16_t)icmp->un.echo.id), pid, RESET);
            return;
        case ERR_ADDR_MISMATCH:
        {
            // keep in a buffer the expected and received sequence numbers and IP addresses for debugging purposes
            char expected_ip_addr[INET_ADDRSTRLEN];
            char received_ip_addr[INET_ADDRSTRLEN];
            char received_ip_packet[INET_ADDRSTRLEN];
            char expected_ip_packet[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from_addr.sin_addr,
                      expected_ip_addr, sizeof(expected_ip_addr));
            inet_ntop(AF_INET, &recv_addr.sin_addr,
                      received_ip_addr, sizeof(received_ip_addr));

            inet_ntop(AF_INET, &ip_hdr->saddr,
                      received_ip_packet, sizeof(received_ip_packet));
            inet_ntop(AF_INET, &ip_hdr->daddr,
                      expected_ip_packet, sizeof(expected_ip_packet));
            printf("%secho reply from unexpected ip :\n    - data in addr src : %s - dst ip : %s\n    - data in packet dst : %s - src : %s%s\n", YELLOW, received_ip_addr, expected_ip_addr, received_ip_packet, expected_ip_packet, RESET);
            return;
        }
        }
    }
    else if (error == ERR_CHECKSUM_INVALID)
    {
        printf("checksum mismatch from %s\n", inet_ntoa(recv_addr.sin_addr));
        return;
    }
}

static bool packet_checker(t_ping *ping, struct icmphdr *icmp, struct sockaddr_in recv_addr, struct iphdr *ip_hdr, int icmp_len)
{
    int error = 0;

    if (is_echo_from_myself(icmp, ping->pid))
    {
        error = ERR_ECHO_FROM_MYSELF;
    }
    else if (icmp->type != ICMP_ECHOREPLY)
    {
        error = ERR_NOT_ECHOREPLY;
    }
    else if (!is_for_my_pid(icmp, ping->pid))
    {
        error = ERR_NOT_MY_PID;
    }
    else if (!is_addr_match(&ping->addr, &recv_addr) && !is_ip_match(ip_hdr->saddr, ip_hdr->daddr))
    {
        error = ERR_ADDR_MISMATCH;
    }
    else if (!verify_checksum(icmp, icmp_len))
    {
        error = ERR_CHECKSUM_INVALID;
    }

    if (error)
    {
        handle_error(error, ping->mode, ping->pid, icmp, ping->addr, recv_addr, ip_hdr);
        return true;
    }
    return false;
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
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
    int icmp_len = result - ip_hdr->ihl * 4;

    printf("result: %d\n", result);
    if (packet_checker(ping, icmp, recv_addr, ip_hdr, icmp_len))
    {
        return true;
    }

    char payload[56] = {0};
    // mettre le time de l'envoi dans le payload et le recupere ici pour calculer le rtt
    memmove(payload, buffer + ip_hdr->ihl * 4 + sizeof(struct icmphdr), sizeof(payload));
    struct timeval *sent_time = (struct timeval *)payload;

    gettimeofday(end_time, NULL);
    double time_packet = (end_time->tv_sec - sent_time->tv_sec) * 1000.0 + (end_time->tv_usec - sent_time->tv_usec) / 1000.0;

    // printf("%d bytes; ", result);
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
