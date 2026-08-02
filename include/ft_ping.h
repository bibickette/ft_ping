#ifndef FT_PING_H
# define FT_PING_H

#define YELLOW "\033[0;33m"
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

enum arg_opt
{
    OPT_VERBOSE  = 1 << 0,
    OPT_COUNT    = 1 << 1,
};

typedef struct s_ping
{
    struct sockaddr_in addr;
    char reverse_dns[1024];
    char *dest;
    int socket_fd;

    int mode;
    int count;
    unsigned long size_payload;

    ssize_t packets_sent;
    ssize_t packets_received;
    
    
} t_ping;

void parse_opts(int argc, char *argv[], t_ping *ping);

/*  do_ping.c */
void resolve_destination(t_ping *ping);
void loop(t_ping *ping);

/*  send.c */
void send_packet(int socket_fd, struct sockaddr_in *addr, ssize_t *sequence_number, char *dest);

/*  receive.c */
void receive_packet(int socket_fd, struct sockaddr_in *addr, ssize_t *packets_received, char *reverse_dns);

#endif