
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

static bool is_sequence_expected(struct icmphdr *receive_data, ssize_t *packets_sent)
{
    return ntohs(receive_data->un.echo.sequence) < *packets_sent;
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

bool receive_packet(t_ping *ping, struct timeval *end_time)
{
    // receive packet
    char buffer[1024] = {0};
    struct sockaddr_in recv_addr = ping->addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    int result = recvfrom(ping->socket_fd, buffer, 1024, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
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
    // 1. verify type and id
    // 2. verify source address
    // 3. verify checksum
    if(is_echo_from_myself(icmp)){
        printf("Received an echo request from myself, ignoring...\n");
        return true;
    }
    if(!is_my_pid(icmp)){
        printf("Received an echo reply not for me, ignoring...\n");
        return true;
    }
    if(!is_sequence_expected(icmp, &ping->packets_sent)){
        printf("Received an echo reply with unexpected sequence number, ignoring...\n");
        return true;
    }
    if(!is_addr_match(&ping->addr, &recv_addr)){
        printf("Received an echo reply from unexpected source, ignoring...\n");
        return true;
    }
    if(!verify_checksum(icmp, icmp_len)){
        printf("Received an echo reply with invalid checksum, ignoring...\n");
        return true;
    }

    // if (!is_echo_from_myself(icmp)
    //     && is_my_pid(icmp) 
    //     && is_addr_match(addr, &recv_addr) 
    //     && verify_checksum(icmp))
    // {

        char payload[56] = {0};
        // mettre le time de l'envoi dans le payload et le recupere ici pour calculer le rtt
        memmove(payload, buffer + ip_hdr->ihl * 4 + sizeof(struct icmphdr), sizeof(payload));
        struct timeval *sent_time = (struct timeval *)payload;
        // inetutils-2.0/ping/ping_common.h:#define MAXWAIT         10     /* Max seconds to wait for response.  */
        // si le  paquet est recu apres 10s il est considere comme perdu et on ne le prend pas en compte pour le calcul du rtt

        gettimeofday(end_time, NULL);
        double time_packet = (end_time->tv_sec - sent_time->tv_sec) * 1000.0 + (end_time->tv_usec - sent_time->tv_usec) / 1000.0;
        if(time_packet > ping->time_for_reply * 1000.0){
            printf("Received an echo reply after %.3f ms, ignoring...\n", time_packet);
            return true;
        }
        fill_rtt(&ping->rtt, time_packet);
        ping->packets_received++;
        printf("%s%d bytes from %s: ", GREEN, result - ip_hdr->ihl * 4, inet_ntoa(recv_addr.sin_addr));
        printf("icmp_seq=%u ", ntohs(((struct icmphdr *)(buffer + ip_hdr->ihl * 4))->un.echo.sequence));
        printf("ttl=%u ", ip_hdr->ttl);
        printf("rtt=%.3f ms%s\n\n", time_packet, RESET);
    // }
    // else{
    //     printf("not my packet, ignoring...\n");
    //         // keep in a buffer the expected and received sequence numbers and IP addresses for debugging purposes
    //         char expected_ip[INET_ADDRSTRLEN];
    //         char received_ip[INET_ADDRSTRLEN];

    //         inet_ntop(AF_INET, &addr->sin_addr,
    //                   expected_ip, sizeof(expected_ip));

    //         inet_ntop(AF_INET, &recv_addr.sin_addr,
    //                   received_ip, sizeof(received_ip));
    //         // end keeping
    //         printf("Expected sequence number %zu ; received %u\n", *packets_received, ntohs(icmp->un.echo.sequence));
    //         printf("Expected source %s ; received %s\n", expected_ip, received_ip);
    //         printf("ID: %u, my PID: %u\n", ntohs(icmp->un.echo.id), htons(getpid() & 0xffff));
    //         printf("Received ICMP type %d code %d\n", icmp->type, icmp->code);

    // }
    return true;
}
