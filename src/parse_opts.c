#include "ft_ping.h"

void parse_opts(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"help",      no_argument, 0, 'h'},
        {"help",      no_argument, 0, '?'},
        {"verbose",  no_argument,       0, 'v'},
        {"count",    required_argument, 0, 'c'},
        {0,          0,                 0,  0 }
    };

    int opt = 0;
    int rest_args = 0;
    while ((opt = getopt_long(argc, argv, "?hc:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
            case '?':
                // check de optopt pour savoir c'est l'option help car si fail, optopt renvoie le char qui a échoué
                if(!optopt) 
                {
                        printf("Option --help ? is set\n");
                        exit(EXIT_SUCCESS);
                }
                exit(EXIT_FAILURE);
            case 'c':
                printf("Option --count with value: %s\n", optarg);
                break;
            case 'v':
                printf("Option --verbose is set\n");
                break;
            default:
                fprintf(stderr, "Usage: %s [--help][--count <value>] [--verbose <value>]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    rest_args= argc - optind;

    if(rest_args > 1){
        fprintf(stderr, "One argument only\n");
        exit(EXIT_FAILURE);
    }
    else if(rest_args != 1){
        fprintf(stderr, "need one argument\n");
        exit(EXIT_FAILURE);
    }

    printf("argint: %d\n", optind);
}
