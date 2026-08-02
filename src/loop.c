#include "ft_ping.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

void resolve_destination(t_ping *ping)
{

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP; // ICMP protocol

    int status = getaddrinfo(ping->dest, NULL, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "%sping: %s: %s%s\n", RED, ping->dest, gai_strerror(status), RESET);
        exit(EXIT_FAILURE);
    }

    memcpy(&ping->addr, res->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(res);

    printf("%sPING %s (%s): %lu bytes of data.%s\n", GREEN, ping->dest, inet_ntoa(ping->addr.sin_addr), ping->size_payload, RESET);

    ping->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ping->socket_fd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC; // 3 seconds timeout
    timeout.tv_usec = 0;
    if (setsockopt(ping->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    // ajouter setsockopt pour le timeout de recvfrom instead of blocking indefinitely

}

void loop(t_ping *ping)
{
    resolve_destination(ping);
    signal(SIGINT, sigint_handler);

    struct timeval start_time, end_time;
    int res = 0;

    while (g_running)
    {
        send_packet(ping->socket_fd, &ping->addr, &ping->packets_sent, ping->dest, &start_time);
        res=receive_packet(ping->socket_fd, &ping->addr, &ping->packets_received, &start_time, &end_time);
        switch (res)
        {
            case TIMEOUT:
                printf("%sRequest timed out.%s\n", RED, RESET);
                break;
            case FAILED:
                return;
            case SUCCESS:
                sleep(1); // Simulate a ping delay
                continue; // Continue to the next iteration without sleeping
        }

        // sleep(1); // Simulate a ping delay
    }
}
