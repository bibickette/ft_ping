#include "ft_ping.h"
#include <signal.h>

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* calcul si le current time - start time est > timeout_sec */
bool is_timeout(struct timeval *start_time, double timeout_millisec)
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    double elapsed_time = (current_time.tv_sec - start_time->tv_sec) * 1000 + (current_time.tv_usec - start_time->tv_usec) / 1000;
    return (elapsed_time >= timeout_millisec);
}

// fd set
// fd zero
// si mon dernier paquet est envoyé a > timeout => envoyer un autre
// res = select
// if res < 0 && errno != EINTR => perror
// si res == 0 -> continue => timeout de select => envoyer un autre paquet
// if res > 0 => receive_packet
// receive packet : si pas mon paquet => continue
// si mon paquet => afficher le rtt et les infos

/*
    calculate remaining time to wait in the select
    each time we send a packet, we need to wait for a response for TIMEOUT_SEC seconds
    so we calculate the remaining time to wait for the select call
    exact behavior of the inetutils ping (see source code)
*/
static void calculate_time_to_wait(struct timeval *start_time, struct timeval *resp_time, ssize_t interval_ms)
{
    struct timeval intvl, now;

    gettimeofday(&now, NULL);

    intvl.tv_sec = interval_ms / MILLISEC_PRECISION;                         // convert milliseconds to seconds
    intvl.tv_usec = (interval_ms % MILLISEC_PRECISION) * MILLISEC_PRECISION; // convert remaining milliseconds to microseconds
    resp_time->tv_sec = start_time->tv_sec + intvl.tv_sec - now.tv_sec;
    resp_time->tv_usec = start_time->tv_usec + intvl.tv_usec - now.tv_usec;

    while (resp_time->tv_usec < 0)
    {
        resp_time->tv_usec += 1000000;
        resp_time->tv_sec--;
    }
    while (resp_time->tv_usec >= 1000000)
    {
        resp_time->tv_usec -= 1000000;
        resp_time->tv_sec++;
    }

    if (resp_time->tv_sec < 0)
    {
        resp_time->tv_sec = resp_time->tv_usec = 0;
    }
}

bool loop(t_ping *ping)
{
    signal(SIGINT, sigint_handler);

    fd_set read_fds;

    struct timeval resp_time, last_send, last_receive, program_start_time;
    int res = 0;
    int finish = 0;
    ssize_t interval_ms = ping->interval_ms;
    gettimeofday(&program_start_time, NULL);
    if (!send_packet(ping, &last_send))
    {
        return false;
    }
    while (g_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(ping->socket_fd, &read_fds);

        if (is_timeout(&last_send, ping->interval_ms) && (ping->count == 0 || ping->count > ping->packets_sent))
        {
            if (!send_packet(ping, &last_send))
            {
                return false;
            }
            if ((ping->timeout_s > 0 && is_timeout(&program_start_time, ping->timeout_s * MILLISEC_PRECISION)))
            {
                break;
            }
        }
        calculate_time_to_wait(&last_send, &resp_time, interval_ms);

        res = select(ping->socket_fd + 1, &read_fds, NULL, NULL, &resp_time);
        if (res < 0)
        {
            if (errno == EINTR)
            {
                break;
            }
            perror("select failed");
            return false;
        }
        else if (res == 0)
        {
            // si select na pas recu de paquet pendant -W temps -> alors cest la fin SI on nenvoie pu de paquet
            // si select recoit rien depuis X temps ET si on a pas de count alors on continue
            // -W peut changer ce temps
            // -W ne controle pas si laller retour est > a timeout, cest juste pour le select
            if ((ping->timeout_s > 0 && is_timeout(&program_start_time, ping->timeout_s * MILLISEC_PRECISION)))
            {
                break;
            }
            if (finish)
            {
                if (is_timeout(&last_send, ping->linger_ms))
                {
                    if (ping->mode & OPT_VERBOSE)
                    {
                        printf("%sLinger timeout (%d milliseconds)%s\n", YELLOW, ping->linger_ms, RESET);
                    }
                    break;
                }
                continue;
            }
            if (ping->packets_sent >= ping->count && ping->count > 0)
            {
                finish = 1;
                printf(" finishing sending waiting for response\n");
                interval_ms = ping->linger_ms;  // if we have sent all packets, we wait for linger time before exiting
                gettimeofday(&last_send, NULL); // reset last_send to now to start the linger timer
            }
            continue;
        }
        else
        {

            if (!receive_packet(ping, &last_receive))
            {
                return false;
            }
            if (ping->count > 0 && ping->count <= (ping->packets_received + ping->duplicates))
            {
                break;
            }
            if ((ping->timeout_s > 0 && is_timeout(&program_start_time, ping->timeout_s * MILLISEC_PRECISION)))
            {
                break;
            }
        }
    }
    return true;
}
