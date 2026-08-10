#include "ft_ping.h"

static void print_help()
{
    printf("Usage\n  ft_ping [options] <destination>\n\n");
    printf("Options\n");
    printf("  <destination>             DNS name or IP address\n");
    printf("  -?, -h, --help            Print help and exit\n");
    printf("  -v, --verbose             Enable verbose output\n");
    printf("  -c, --count <value>       to define\n");
    printf("  -W, --linger <value>      to define\n");
}

void parse_opts(int argc, char *argv[], t_ping *ping)
{
    static struct option long_options[] = {
        {"help", no_argument, 0, '?'},
        {"verbose", no_argument, 0, 'v'},
        {"count", required_argument, 0, 'c'},
        {"linger", required_argument, 0, 'W'},
        {"timeout", required_argument, 0, 'w'},
        {"ttl", required_argument, 0, 't'},
        {"interval", required_argument, 0, 'i'},
        {0, 0, 0, 0}};

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "?hc:vW:w:t:i:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'h':
        case '?':
            print_help();
            exit(EXIT_FAILURE);
        case 'v':
            ping->mode |= OPT_VERBOSE;
            break;
        case 'c':

            ping->count = atoi(optarg);
            break;
        case 't':
            ping->ttl = atoi(optarg);
            ping->mode |= OPT_TTL;
            // opt between 1 and 255, otherwise exit with error
            /*
            ➜  ft_ping git:(main) ✗ ping localhost --ttl=256
3ping: option value too big: 256
➜  ft_ping git:(main) ✗ ping localhost --ttl=0
ping: option value too small: 0
*/
            printf("Option --ttl with value: %s\n", optarg);
            break;

        case 'W':
            ping->linger = atoi(optarg);
            // max = int max and min 1
            /*
            ➜  ft_ping git:(main) ✗ ping localhost -W  2147483648
ping: option value too big: 2147483648
➜  ft_ping git:(main) ✗ ping localhost -W  2147483647
➜  ft_ping git:(main) ✗ ping localhost -W  0
ping: option value too small: 0
*/
            break;
        case 'w':
            // same than -W
            ping->timeout = atoi(optarg);
            break;
        case 'i':
        {
            char *end; // to verify if interval is a valid number
            double interval = strtod(optarg, &end);
            if (interval < 0.2 || *end != '\0')
            {
                fprintf(stderr, "%sping: option value too small: %s%s\n", RED, optarg, RESET);
                printf("end: %s\n", end);
                exit(EXIT_FAILURE);
            }
        }
        // interval between each ping, min 0.2s
        break;
            // il ny a pas de defaut car si ca foire cest getoptlong qui renvoie ? et on catch ca dans le case '?' sinon il quitte lui meme
        }
    }

    int rest_args = argc - optind;
    if (!rest_args)
    {
        fprintf(stderr, "%sft_ping: usage error: Destination address required%s\n", RED, RESET);
        exit(EXIT_FAILURE);
    }
    else if (rest_args > 1)
    {
        fprintf(stderr, "%sft_ping: usage error: Operation not permitted%s\n", RED, RESET);
        exit(EXIT_FAILURE);
    }

    // optind is the index of the first non-option argument
    ping->dest = argv[optind];
}
