#include "ft_ping.h"
#include <math.h>

static double calculate_stddev(t_rtt *rtt, ssize_t packets_received, double avg_time)
{
    if (packets_received == 0)
    {
        return 0.0;
    }
    double variance = (rtt->total_squared / packets_received) - (avg_time * avg_time);
    if (variance < 0.0)
    {
        variance = 0.0;
    }
    return sqrt(variance);
}

void print_stats(t_ping *ping)
{
    printf("--- %s ping statistics ---\n", ping->dest);
    printf("%ld packets transmitted, %ld received, ", ping->packets_sent, ping->packets_received);
    if (ping->duplicates > 0)
    {
        printf("+%d duplicates, ", ping->duplicates);
    }
    printf("%ld%% packet loss\n", (ping->packets_sent - ping->packets_received) * 100 / ping->packets_sent);

    double avg_time = 0.0;
    if (ping->packets_received != 0)
    {
        avg_time = ping->rtt.total / (ping->packets_received + ping->duplicates);
    }
    
    double stddev = calculate_stddev(&ping->rtt, ping->packets_received + ping->duplicates, avg_time);
    if ((ping->packets_received + ping->duplicates) != 0)
    {
        printf("round-trip min/avg/max/stddev = ");
        printf("%.3f/", ping->rtt.min);
        printf("%.3f/", avg_time);
        printf("%.3f/", ping->rtt.max);
        printf("%.3f ms\n", stddev);
    }
}
