
#include "ft_ping.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

enum packet_error
{
    ERR_ECHO_FROM_MYSELF = 1,
    ERR_NOT_MY_PID = 2,
    ERR_ADDR_MISMATCH = 3,
    ERR_CHECKSUM_INVALID = 4,
    ERR_NOT_ECHOREPLY = 5
};

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

static char *icmp_type_to_string(uint8_t type)
{
    switch (type)
    {
    case ICMP_DEST_UNREACH:
        return "Destination Unreachable";
    case ICMP_TIME_EXCEEDED:
        return "Time Exceeded";
    case ICMP_PARAMETERPROB:
        return "Parameter Problem";
    default:
        return "Unknown Type";
    }
}

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

static void handle_wrong_type(t_ping *ping, int result, char *buffer)
{
    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
    struct iphdr *orig_ip_hdr = (struct iphdr *)((unsigned char *)icmp + sizeof(struct icmphdr));
    struct icmphdr *orig_icmp = (struct icmphdr *)((unsigned char *)orig_ip_hdr + sizeof(struct iphdr));

    if (!is_for_my_pid(orig_icmp, ping->pid))
    {
        return;
    }
    if (icmp->type == ICMP_DEST_UNREACH || icmp->type == ICMP_TIME_EXCEEDED || icmp->type == ICMP_PARAMETERPROB)
    {
        ping->packets_error++; // increment lost packets count since we received a non-echo reply packet
        printf("%d bytes from %s: %s\n", result - ip_hdr->ihl * 4, inet_ntoa(*(struct in_addr *)&ip_hdr->saddr), icmp_type_to_string(icmp->type));
        if(!(ping->mode & OPT_VERBOSE))
        {
            return;
        }        
        printf("IP Hdr Dump:\n ");
        for (int i = 0; i < 20 && i < result; i++)
        {
            printf("%02x", (unsigned char)buffer[i]);
            if (i % 2 == 1)
                printf(" ");
        }
        char src[INET_ADDRSTRLEN];
        char dst[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &orig_ip_hdr->saddr, src, sizeof(src));
        inet_ntop(AF_INET, &orig_ip_hdr->daddr, dst, sizeof(dst));

        printf("\nVr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");
        printf(" %1x  %1x  %02x", orig_ip_hdr->version, orig_ip_hdr->ihl, orig_ip_hdr->tos);
        printf(" %04x %04x", ntohs(orig_ip_hdr->tot_len), ntohs(orig_ip_hdr->id));
        printf("   %1x %04x", (ntohs(orig_ip_hdr->frag_off) & 0xe000) >> 13, ntohs(orig_ip_hdr->frag_off) & 0x1fff);
        printf("  %02x  %02x %04x", orig_ip_hdr->ttl, orig_ip_hdr->protocol, ntohs(orig_ip_hdr->check));
        printf(" %s ", src);
        printf(" %s ", dst);
        printf("\n");
        printf("ICMP: type: %d, code: %d, size: %d, id: 0x%04x, seq: 0x%04x\n", orig_icmp->type, orig_icmp->code, ntohs(orig_ip_hdr->tot_len) - orig_ip_hdr->ihl * 4, ntohs(orig_icmp->un.echo.id), ntohs(orig_icmp->un.echo.sequence));
    }
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

    if (icmp->type != ICMP_ECHOREPLY && icmp->type != ICMP_ECHO)
    {
        handle_wrong_type(ping, result, buffer);
        return true;
    }
    else if (packet_checker(ping, icmp, recv_addr, ip_hdr, icmp_len))
    {
        return true;
    }

    char payload[56] = {0};
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
