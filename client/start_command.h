#ifndef _START_COMMAND
#define _START_COMMAND 1

struct start_command {
    char command[8]; /* "start\0" */
    long n_packet;
    int  packet_size;
};

#endif
