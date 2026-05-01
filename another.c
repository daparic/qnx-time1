/*
 *  another.c
 *
 *  Client that connects to the time1 server and sends a message.
 *
 *  Usage:
 *    another <server-pid> <channel-id> wait
 *    another <server-pid> <channel-id> send [data]
 *
 *  The server PID and channel ID are printed by time1 on startup when
 *  debug is enabled, or can be retrieved with:
 *    pidin -p time1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include "protocol.h"

int
main (int argc, char *argv[])
{
    pid_t           server_pid;
    int             server_chid;
    int             coid;
    ClientMessageT  smsg;
    ClientMessageT  rmsg;
    int             status;

    if (argc < 4) {
        fprintf (stderr, "Usage: %s <server-pid> <channel-id> wait|send [data]\n",
                 argv[0]);
        return EXIT_FAILURE;
    }

    server_pid  = (pid_t) atoi (argv[1]);
    server_chid =         atoi (argv[2]);

    /* attach to the server's channel */
    coid = ConnectAttach (0, server_pid, server_chid, _NTO_SIDE_CHANNEL, 0);
    if (coid == -1) {
        fprintf (stderr, "ConnectAttach to pid=%d chid=%d failed: %s\n",
                 server_pid, server_chid, strerror (errno));
        return EXIT_FAILURE;
    }

    memset (&smsg, 0, sizeof (smsg));

    if (strcmp (argv[3], "wait") == 0) {
        smsg.messageType = MT_WAIT_DATA;
        printf ("Sending MT_WAIT_DATA to pid=%d chid=%d ...\n",
                server_pid, server_chid);

    } else if (strcmp (argv[3], "send") == 0) {
        smsg.messageType = MT_SEND_DATA;
        smsg.messageData = (argc >= 5) ? atoi (argv[4]) : 0;
        printf ("Sending MT_SEND_DATA data=%d to pid=%d chid=%d ...\n",
                smsg.messageData, server_pid, server_chid);

    } else {
        fprintf (stderr, "Unknown message type '%s': use 'wait' or 'send'\n",
                 argv[3]);
        ConnectDetach (coid);
        return EXIT_FAILURE;
    }

    /* blocks until the server calls MsgReply */
    status = MsgSend (coid, &smsg, sizeof (smsg), &rmsg, sizeof (rmsg));
    if (status == -1) {
        fprintf (stderr, "MsgSend failed: %s\n", strerror (errno));
        ConnectDetach (coid);
        return EXIT_FAILURE;
    }

    switch (rmsg.messageType) {
    case MT_OK:
        printf ("Reply: MT_OK  data=%d\n", rmsg.messageData);
        break;
    case MT_TIMEDOUT:
        printf ("Reply: MT_TIMEDOUT\n");
        break;
    default:
        printf ("Reply: unknown type %d\n", rmsg.messageType);
        break;
    }

    ConnectDetach (coid);
    return EXIT_SUCCESS;
}
