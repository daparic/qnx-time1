#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/neutrino.h>

/* messages from client to server */
#define MT_WAIT_DATA    2
#define MT_SEND_DATA    3

/* pulse codes */
#define CODE_TIMER      1

/* replies from server to client */
#define MT_OK           0
#define MT_TIMEDOUT     1

typedef struct
{
    int messageType;
    int messageData;
} ClientMessageT;

typedef union
{
    ClientMessageT  msg;
    struct _pulse   pulse;
} MessageT;

#endif /* PROTOCOL_H */
