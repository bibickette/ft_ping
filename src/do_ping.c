#include "ft_ping.h"

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig){
    (void)sig;
    g_running = 0;
}

void do_ping(t_ping *ping) {    
    signal(SIGINT, sigint_handler);

    while(g_running) {
        printf("Pinging %s...\n", ping->dest);
        sleep(1); // Simulate a ping delay
    }
}
