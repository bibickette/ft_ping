#include "ft_ping.h"

static void print_help() {
    printf("Usage\n  ft_ping [options] <destination>\n\n");
    printf("Options\n");
    printf("  <destination>             DNS name or IP address\n");
    printf("  -?, -h, --help            Print help and exit\n");
    printf("  -v, --verbose             Enable verbose output\n");
    printf("  -c, --count <value>       Stop after <count> replies\n");
}



void parse_opts(int argc, char *argv[], t_ping *ping) {
    static struct option long_options[] = {
        {"help",     no_argument,       0, '?'},
        {"verbose",  no_argument,       0, 'v'},
        {"count",    required_argument, 0, 'c'},
        {0,          0,                 0,  0 }
    };

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "?hc:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
            case '?':
                print_help();
                exit(EXIT_FAILURE);
            case 'c':
                ping->count = atoi(optarg);
                if(ping->count > 0)
                {
                    ping->mode |= OPT_COUNT;
                }
                printf("Option --count with value: %s\n", optarg);
                break;
            case 'v':
                ping->mode |= OPT_VERBOSE;
                break;
            case 's':
                ping->size_payload = atoi(optarg);
                printf("Option --size with value: %s\n", optarg);
                break;
            // il ny a pas de defaut car si ca foire cest getoptlong qui renvoie ? et on catch ca dans le case '?' sinon il quitte lui meme
        }
    }

    int rest_args = argc - optind;
    if(!rest_args){
        fprintf(stderr, "%sft_ping: usage error: Destination address required%s\n", RED, RESET);
        exit(EXIT_FAILURE);
    }
    else if(rest_args > 1){
        fprintf(stderr, "%sft_ping: usage error: Operation not permitted%s\n", RED, RESET);
        exit(EXIT_FAILURE);
    }

    // optind is the index of the first non-option argument
    ping->dest = argv[optind];

}


