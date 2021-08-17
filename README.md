# packet flow

```
  C                     S
  |                     |
  | start command (UDP) |
  |-------------------->|
  |                     |
  |  data packet (UDP)  | (packet_size)
  |<--------------------|
  |<--------------------|
  |<--------------------|
  |<--------------------|
  |<--------------------|
  |<--------------------|
  |<--------------------| (total n_packet)
  |                     |
```

```
struct start_command {
    char command[8]; /* "start\0" */
    long n_packet;
    int  packet_size;
};
```

port number: 1234 (UDP)
