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
/* linger time default */
// inetutils-2.0/ping/ping_common.h:#define MAXWAIT         10
#define LINGER_TIME 10
/* maximum number of received sequence numbers to save, permet to control duplicates */
#define MAX_RECV_SEQ_SAVE 1024

/* buffer size to receive */
#define RECV_BUFFER_SIZE 1024
/* payload size in icmp packet */
#define PAYLOAD_SIZE 56

enum arg_opt
{
    OPT_VERBOSE  = 1 << 0,
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
    uint16_t pid;
    
    /* flags */
    int mode;
    ssize_t count; // -c
    int linger; // -W
    int timeout; // -w

    unsigned long size_payload;
    /* garder le sequence number dans un uint16 car cest le type du paquet de sequence, donc quand on envoie  > uint16 seq il reboucle*/
    uint16_t sequence_number;
    /* tableau pour suivre les séquences reçues pour controler les dupplicate*/
    bool sequence_received[MAX_RECV_SEQ_SAVE];

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
bool is_duplicate(t_ping *ping, uint16_t sequence_number);


#endif