#include "ft_ping.h"


int main(int argc, char *argv[])
{

    t_ping ping;
    memset(&ping, 0, sizeof(t_ping));

    parse_opts(argc, argv, &ping);

    
    if (ping.mode & OPT_VERBOSE)
    {
        printf("Verbose mode enabled\n");
    }
    if (ping.mode & OPT_COUNT)
    {
        printf("Count mode enabled\n");
    }

    resolve_destination(&ping);

    do_ping(&ping);

    printf("\n%sExiting ft_ping...%s\n", YELLOW, RESET);
    return 0;
}