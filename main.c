#include "ft_ping.h"

int main(int argc, char *argv[]) {

    t_ping ping;
    memset(&ping, 0, sizeof(t_ping));

    parse_opts(argc, argv, &ping);

    printf("==== Destination: %s\n", ping.dest);
    if (ping.mode & OPT_VERBOSE) {
        printf("Verbose mode enabled\n");
    }
    if (ping.mode & OPT_COUNT) {
        printf("Count mode enabled\n");
    }

    do_ping(&ping);

    // printf("Exiting ft_ping...\n");
    return 0;
}