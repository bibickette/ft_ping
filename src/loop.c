#include "ft_ping.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

static volatile sig_atomic_t g_running = 1;

void sigint_handler(int sig)
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

    memmove(&ping->addr, res->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(res);

    printf("PING %s (%s): %u bytes of data", ping->dest, inet_ntoa(ping->addr.sin_addr), PAYLOAD_SIZE);
    if (ping->mode & OPT_VERBOSE)
    {
        int pid = getpid() & 0xFFFF;
        printf(", id 0x%x = %i\n", pid, pid);
    }
    else
    {

        printf(".\n");
    }
    ping->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ping->socket_fd < 0)
    {
        if (errno == EPERM || errno == EACCES)
        {
            if(ping->mode & OPT_VERBOSE)
            {
                printf("%sRoot privileges required for raw socket, using SOCK_DGRAM instead%s\n", YELLOW, RESET);
            }
            ping->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
            if (ping->socket_fd < 0)
            {
                perror("socket_dgram creation failed : ");
                exit(EXIT_FAILURE);
            }
            ping->socket_dgram = true;
        }
        else{
            perror("socket_raw creation failed : ");
            exit(EXIT_FAILURE);
        }
    }
}

/* calcul si le current time - start time est > timeout_sec */ 
bool is_timeout(struct timeval *start_time, int timeout_sec)
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    long elapsed_time = (current_time.tv_sec - start_time->tv_sec) * 1000 + (current_time.tv_usec - start_time->tv_usec) / 1000;
    return elapsed_time >= timeout_sec * 1000;
}

// fd set
// fd zero
// si mon dernier paquet est envoyé a > timeout => envoyer un autre
// res = select
// if res < 0 && errno != EINTR => perror
// si res == 0 -> continue => timeout de select => envoyer un autre paquet
// if res > 0 => receive_packet
// receive packet : si pas mon paquet => continue
// si mon paquet => afficher le rtt et les infos
bool loop(t_ping *ping)
{
    resolve_destination(ping);
    signal(SIGINT, sigint_handler);

    fd_set read_fds;

    struct timeval timeout, start_time, end_time;
    int res = 0;

    if (!send_packet(ping, &start_time))
    {
        return false;
    }
    while (g_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(ping->socket_fd, &read_fds);

        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;

        if (is_timeout(&start_time, TIMEOUT_SEC))
        {
            if (ping->count == 0 || ping->count > ping->packets_sent)
            {
                if (!send_packet(ping, &start_time))
                {
                    return false;
                }
            }
        }
        res = select(ping->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (res < 0)
        {
            if (errno == EINTR)
            {
                break;
            }
            perror("select failed");
            return false;
        }
        else if (res == 0)
        {
            // si select na pas recu de paquet pendant -W temps -> alors cest la fin SI on nenvoie pu de paquet
            // si select recoit rien depuis X temps ET si on a pas de count alors on continue
            // -W peut changer ce temps
            // -W ne controle pas si laller retour est > a timeout, cest juste pour le select
            // inetutils-2.0/ping/ping_common.h:#define MAXWAIT         10     /* Max seconds to wait for response.  */
            if (is_timeout(&start_time, ping->time_select))
            {
                printf("Nothing happened for %d seconds, exiting...\n", ping->time_select);
                break;
            }
            continue;
        }
        else
        {
            if (!receive_packet(ping, &end_time))
            {
                return false;
            }
            if (ping->count > 0 && ping->count <= (ping->packets_received + ping->duplicates))
            {
                break;
            }
        }
    }
    return true;
}
