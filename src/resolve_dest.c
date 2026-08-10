#include "ft_ping.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

static void print_destination(t_ping *ping)
{
    printf("PING %s (%s): %u bytes of data", ping->dest, inet_ntoa(ping->addr.sin_addr), PAYLOAD_SIZE);
    if (ping->mode & OPT_VERBOSE)
    {
        printf(", id 0x%x = %i\n", ping->pid, ping->pid);
    }
    else
    {
        printf(".\n");
    }
}

static int create_socket(void)
{
    int fd = 0;
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd < 0)
    {
        perror("socket_raw creation failed : ");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void resolve_dest(t_ping *ping)
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

    memmove(&ping->addr, res->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(res);

    ping->socket_fd = create_socket();

    if (ping->mode & OPT_TTL)
    {
        if (setsockopt(ping->socket_fd, IPPROTO_IP, IP_TTL, &ping->ttl, sizeof(int)) < 0)
        {
            close(ping->socket_fd);
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }
    }

    print_destination(ping);
}
