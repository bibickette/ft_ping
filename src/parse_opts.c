#include "ft_ping.h"

static void print_help() {
    printf("Usage\n  ft_ping [options] <destination>\n\n");
    printf("Options\n");
    printf("  <destination>             DNS name or IP address\n");
    printf("  -?, -h, --help            Print help and exit\n");
    printf("  -v, --verbose             Enable verbose output\n");
    printf("  -c, --count <value>       to define\n");
    printf("  -W, --linger <value>      to define\n");
}



void parse_opts(int argc, char *argv[], t_ping *ping) {
    static struct option long_options[] = {
        {"help",     no_argument,       0, '?'},
        {"verbose",  no_argument,       0, 'v'},
        {"count",    required_argument, 0, 'c'},
        {"linger",   required_argument, 0, 'W'},
        {"timeout",  required_argument, 0, 'w'},
        {0,          0,                 0,  0 }
    };

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "?hc:vW:w:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
            case '?':
                print_help();
                exit(EXIT_FAILURE);
            case 'c':
                ping->count = atoi(optarg);
                break;
            case 'v':
                ping->mode |= OPT_VERBOSE;
                break;
            case 'W':
                ping->linger = atoi(optarg);
                break;
            case'w':
                ping->timeout = atoi(optarg);
                break;
            // case 's':
            //     ping->size_payload = atoi(optarg);
            //     printf("Option --size with value: %s\n", optarg);
            //     break;
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


