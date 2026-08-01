#ifndef FT_PING_H
# define FT_PING_H

#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

enum arg_opt
{
    OPT_VERBOSE  = 1 << 0,
    OPT_COUNT    = 1 << 1,
};

typedef struct s_ping
{
    char *dest;

    int mode;
    int count;
    
} t_ping;

void parse_opts(int argc, char *argv[], t_ping *ping);

#endif