#include "ft_ping.h"

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig){
    (void)sig; // Unused parameter
    g_running = 0;
}
int main(int argc, char *argv[]) {
    t_ping ping;

    memset(&ping, 0, sizeof(t_ping));

    parse_opts(argc, argv, &ping);


    printf("==== Destination: %s\n", ping.dest);
    printf("mode: %d\n", ping.mode);
    if (ping.mode & OPT_VERBOSE) {
        printf("Verbose mode enabled\n");
    }
    if (ping.mode & OPT_COUNT) {
        printf("Count mode enabled\n");
    }

    signal(SIGINT, sigint_handler);

    while(g_running) {
        // Main ping loop
        printf("Pinging %s...\n", ping.dest);
        sleep(1); // Simulate a ping delay
    }
    return 0;
}