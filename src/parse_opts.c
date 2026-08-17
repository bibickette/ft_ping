#include "ft_ping.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static void print_help()
{
    printf("Usage\n  ft_ping [options] <destination>\n\n");
    printf("Options\n");
    printf("  <destination>             DNS name or IP address\n");
    printf("  -?, -h, --help            Print help and exit\n");
    printf("  -v, --verbose             Enable verbose output\n");
    printf("  -c, --count <value>       Number of packets to send\n");
    printf("  -W, --linger <value>      Number of seconds to keep waiting for pending replies, once all packets have already been sent\n");
    printf("  -w, --timeout <value>     Stop the whole program after N seconds have elapsed, regardless of how many packets were sent\n");
    printf("  -t, --ttl <value>         Time to live for each packet\n");
    printf("  -i, --interval <value>    Wait NUMBER seconds between sending each packet\n");
}

static bool is_between_int(char *arg, int min, int max)
{

    char *end;
    errno = 0;
    int n = -1;

    long value_long = strtol(arg, &end, 10);

    if (end == arg)
    {
        return false;
    }
    else if (*end != '\0')
    {
        return false;
    }
    else if (errno == ERANGE || value_long < INT_MIN || value_long > INT_MAX)
    {
        return false;
    }

    n = (int)value_long;

    return (n >= min && n <= max);
}

static bool is_between_double(char *arg, double min, double max)
{
    char *end;
    errno = 0;
    double n = -1;

    double value_double = strtod(arg, &end);

    if (end == arg)
    {
        return false;
    }
    else if (*end != '\0')
    {
        return false;
    }
    else if (errno == ERANGE || value_double < min || value_double > max)
    {
        return false;
    }

    n = value_double;

    return (n >= min && n <= max);
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
        {
            print_help();
            exit(EXIT_FAILURE);
        }
        case 'v':
        {
            ping->mode |= OPT_VERBOSE;
            break;
        }
        case 'c':
        {
            if (!is_between_int(optarg, 1, INT_MAX))
            {
                fprintf(stderr, "%sping: option value must be between %d and %d%s\n", RED, 1, INT_MAX, RESET);
                exit(EXIT_FAILURE);
            }
            ping->count = atol(optarg);
            break;
        }
        case 't':
        {
            if (!is_between_int(optarg, 1, 255))
            {
                fprintf(stderr, "%sping: option value must be between %d and %d%s\n", RED, 1, 255, RESET);

                exit(EXIT_FAILURE);
            }
            ping->ttl = atoi(optarg);
            ping->mode |= OPT_TTL;
            break;
        }
        case 'W':
        {
            if (!is_between_int(optarg, 1, INT_MAX))
            {
                fprintf(stderr, "%sping: option value must be between %d and %d%s\n", RED, 1, INT_MAX, RESET);
                exit(EXIT_FAILURE);
            }
            ping->linger_ms = atol(optarg) * MILLISEC_PRECISION; // convert to milliseconds
            break;
        }
        case 'w':
        {
            if (!is_between_int(optarg, 1, INT_MAX))
            {
                fprintf(stderr, "%sping: option value must be between %d and %d%s\n", RED, 1, INT_MAX, RESET);
                exit(EXIT_FAILURE);
            }
            ping->timeout_s = atol(optarg);
            break;
        }
        case 'i':
        {
            if (!is_between_double(optarg, 0.2, 20000.0)) // arbitrary max value
            {
                fprintf(stderr, "%sping: option value must be between %f and %f seconds%s\n", RED, 0.2, 20000.0, RESET);
                exit(EXIT_FAILURE);
            }
            double interval = strtod(optarg, NULL);
            ping->interval_ms = interval * MILLISEC_PRECISION; // convert to milliseconds
        }
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
