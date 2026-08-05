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
#include <sys/time.h>
#include <errno.h>

/* time to and select */
#define TIMEOUT_SEC 1
/* time to wait for a reply */
#define LATE_REPLY 10

enum arg_opt
{
    OPT_VERBOSE  = 1 << 0,
    OPT_COUNT    = 1 << 1,
};

typedef struct s_rtt
{
    double min;
    double max;
    double total;
    double total_squared; // for variance calculation

} t_rtt;

typedef struct s_ping
{
    struct sockaddr_in addr;

    char *dest;
    int socket_fd;

    int mode;
    int count;

    int time_select;
    unsigned long size_payload;

    uint16_t sequence_number;
    uint16_t duplicates;

    ssize_t packets_sent;
    ssize_t packets_received;

    t_rtt rtt;
    
} t_ping;



void parse_opts(int argc, char *argv[], t_ping *ping);

/*  do_ping.c */
void resolve_destination(t_ping *ping);
bool loop(t_ping *ping);

/*  send.c */
unsigned short calculate_checksum(unsigned short *data, int count);
bool send_packet(t_ping *ping, struct timeval *start_time);

/*  receive.c */
bool receive_packet(t_ping *ping, struct timeval *end_time);


#endif