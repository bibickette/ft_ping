#include "ft_ping.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig){
    (void)sig;
    g_running = 0;
}


void resolve_destination(t_ping *ping)
{

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 
    hints.ai_socktype = SOCK_RAW;

    int status = getaddrinfo(ping->dest, NULL, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "%sft_ping :    %s  - No address associated with hostname%s\n", RED, ping->dest, RESET);
        exit(EXIT_FAILURE);
    }

    // res->ai_addr contient maintenant la struct sockaddr avec l'IP résolue
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    // free le res apres utilisation
    freeaddrinfo(res);

    printf("%sResolved IP address for %s - %s%s\n", GREEN, ping->dest, inet_ntoa(addr->sin_addr), RESET);
    (void)addr;       
}


void do_ping(t_ping *ping) {    
    signal(SIGINT, sigint_handler);

    while(g_running) {
        printf("Pinging %s...\n", ping->dest);
        sleep(1); // Simulate a ping delay
    }
}
