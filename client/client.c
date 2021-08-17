#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "my_socket.h"
#include "get_num.h"
#include "logUtil.h"
#include "timespecsub.h"
#include "start_command.h"

int debug = 0;

int usage()
{
    char msg[] = "Usage: client [-d] [-h] [-n n_packet] [-s packet_size] [-r so_rcvbuf] server_ip_address";
    fprintf(stderr, "%s\n", msg);

    return 0;
}

int main(int argc, char *argv[])
{
    struct start_command start_command;
    strncpy(start_command.command, "start", sizeof(start_command.command));
    start_command.n_packet = 10;        // default 10 packet
    start_command.packet_size = 1*1024; // default 1kB
    int so_rcvbuf = -1;

    int c;
    while ( (c = getopt(argc, argv, "dhn:r:s:")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'n':
                start_command.n_packet = strtol(optarg, NULL, 0);
                break;
            case 'r':
                so_rcvbuf = get_num(optarg);
                break;
            case 's':
                start_command.packet_size = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage();
        exit(1);
    }

    int port = 1234;
    char *remote_host_info = argv[0];
    char *tmp = strdup(remote_host_info);
    char *remote_host = strsep(&tmp, ":");
    if (tmp != NULL) {
        port = strtol(tmp, NULL, 0);
    }
    if (debug) {
        fprintf(stderr, "remote_host: %s, port: %d\n", remote_host, port);
    }

    int sockfd = udp_socket();
    if (sockfd < 0) {
        err(1, "socket");
    }

    if (so_rcvbuf > 0) {
        if (set_so_rcvbuf(sockfd, so_rcvbuf) < 0) {
            exit(1);
        }
    }

    if (connect_udp(sockfd, remote_host, port) < 0) {
        exit(1);
    }

    int n = write(sockfd, &start_command, sizeof(start_command));
    if (n < 0) {
        err(1, "write() start_command");
    }

    int bufsize = start_command.packet_size;
    unsigned char *buf = malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc for buf");
    }
    memset(buf, 0, bufsize);

    //struct timespec ts_prev, ts_now, ts_diff;
    //ts_prev.tv_sec  = 0;
    //ts_prev.tv_nsec = 0;
    struct timespec *ts_buf = malloc(sizeof(struct timespec)*start_command.n_packet);
    if (ts_buf == NULL) {
        err(1, "malloc for ts_buf");
    }
    memset(ts_buf, 0, sizeof(struct timespec)*start_command.n_packet);

    long last_index = 0;
    for (long i = 0; i < start_command.n_packet; ++i) {
        int n = read(sockfd, buf, bufsize);
        if (n < 0) {
            err(1, "read");
        }
        if (debug) {
            fprintfwt(stderr, "%ld\n", i);
        }
        int *p = (int *)&buf[0];
        if (*p != i) {
            // invalid sequence number. exit.
            fprintf(stderr, "sequence number mismatch. expect: %ld, receive: %d\n", i, *p);
            // exit(0);
            break;
        }
        struct timespec *ts_p;
        ts_p = (struct timespec *)&buf[sizeof(int)];
        // ts_now = *ts_p;
        ts_buf[i] = *ts_p;

        last_index = i;
    }

    //for (long i = 0; i < start_command.n_packet - 1; ++i) {
    for (long i = 0; i < last_index - 1; ++i) {
        struct timespec diff;
        timespecsub(&ts_buf[i+1], &ts_buf[i], &diff);
        printf("%ld.%09ld %ld.%09ld\n", 
            ts_buf[i].tv_sec, ts_buf[i].tv_nsec,
            diff.tv_sec, diff.tv_nsec);
    }

    return 0;
    
}
