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
#include "../client/start_command.h"

int debug = 0;

int usage()
{
    char msg[] = "Usage: server [-d] [-h] [-s so_sndbuf]";
    fprintf(stderr, "%s\n", msg);
    
    return 0;
}

struct start_command decode_command_packet(unsigned char *buf, int buflen)
{
    struct start_command start_command;
    strcpy(start_command.command, (char *) &buf[0]);
    long *long_p = (long *)&buf[8];
    start_command.n_packet = *long_p;
    int *int_p = (int *)&buf[16];
    start_command.packet_size = *int_p;

    return start_command;
}

int main(int argc, char *argv[])
{
    int port = 1234;
    int c;
    int so_sndbuf = -1;
    while ( (c = getopt(argc, argv, "dhp:")) != -1) {
        switch (c) {
            case 'd':
                debug += 1;
                break;
            case 'h':
                usage();
                exit(0);
                break; /* NOTREACHED */
            case 'p':
                port = strtol(optarg, NULL, 0);
                break;
            case 's':
                so_sndbuf = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;
            
    if (debug) {
        fprintf(stderr, "port: %d\n", port);
        fprintf(stderr, "so_sndbuf: %d bytes\n", so_sndbuf);
    }

    for ( ; ; ) { // iteration server.  does not allow simultaneous connction
        int sockfd = udp_socket();
        if (sockfd < 0) {
            err(1, "sockfd");
        }

        if (so_sndbuf > 0) {
            if (set_so_sndbuf(sockfd, so_sndbuf) < 0) {
                exit(1);
            }
        }

        if (my_bind(sockfd, "0.0.0.0", port) < 0) {
            exit(1);
        }

        struct start_command start_command;
        // read command packet
        unsigned char start_command_buf[sizeof(start_command)];
        struct sockaddr_in cliaddr;
        memset(&cliaddr, 0, sizeof(cliaddr));
        socklen_t len = sizeof(cliaddr);
        int n = recvfrom(sockfd, start_command_buf, sizeof(start_command_buf), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            err(1, "recvfrom() start_command");
        }
        if (debug) {
            fprintf(stderr, "recvfrom returns: %d\n", n);
            char ip_address[128];
            inet_ntop(AF_INET, &cliaddr.sin_addr, ip_address, sizeof(ip_address));
            fprintf(stderr, "port: %d\n", ntohs(cliaddr.sin_port));
            fprintf(stderr, "remote_host: %s\n", ip_address);
        }

        start_command = decode_command_packet(start_command_buf, sizeof(start_command_buf));

        printf("command: %s, n_packet: %ld, packet_size: %d\n",
            start_command.command, start_command.n_packet, start_command.packet_size);

        int bufsize = start_command.packet_size;
        unsigned char *buf = malloc(bufsize);
        if (buf == NULL) {
            err(1, "malloc for buf");
        }
        memset(buf, 'X', bufsize);

        for (int i = 0; i < start_command.n_packet; ++i) {
            int *p = (int *)&buf[0];
            *p = i; // sequence number
            struct timespec *ts = (struct timespec *)&buf[sizeof(int)];
            clock_gettime(CLOCK_REALTIME, ts);
            int n = sendto(sockfd, buf, bufsize, 0, (struct sockaddr *)&cliaddr, len);
            if (n < 0) {
                err(1, "sendto");
            }
            if (debug) {
                fprintfwt(stderr, "%d\n", i);
            }
        }
        if (close(sockfd) < 0) {
            err(1, "close");
        }
    }
    
    return 0;
}
