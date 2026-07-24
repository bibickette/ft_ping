#include <stdio.h>
#include <stdbool.h>

enum arg_opt
{
    OPT_TTL      = 1 << 0,
    OPT_INTERVAL = 1 << 1,
    OPT_COUNT    = 1 << 2,
    OPT_TIMEOUT  = 1 << 3,
    OPT_LINGER   = 1 << 4,
    OPT_VERBOSE  = 1 << 5,
};

// getopt_long pour checker les options de la ligne de commande

int main(int argc, char *argv[]) {
    // if(!check_argc(argc, argv)) {
    //     return 1;
    // }
    (void)argc;
    (void)argv;

    int mode =  (OPT_TTL | OPT_COUNT | OPT_INTERVAL);
    if (mode & OPT_TTL) {
        printf("OPT_TTL is set\n");
    }
    if (mode & OPT_COUNT) {
        printf("OPT_COUNT is set\n");
    }
    if (mode & OPT_VERBOSE) {
        printf("OPT_VERBOSE is set\n");
    }
    if (mode & OPT_INTERVAL) {
        printf("OPT_INTERVAL is set\n");
    }
    if (mode & OPT_TIMEOUT) {
        printf("OPT_TIMEOUT is set\n");
    }

    printf("OPT_TTL | OPT_COUNT | OPT_VERBOSE: %d\n", (OPT_TTL | OPT_COUNT | OPT_VERBOSE));
    printf("Hello, World!\n");
    return 0;
}