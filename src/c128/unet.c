/*******************************************************************************
Simple API for outbound TCP and UDP connections using the Ultimate II ethernet
interface.

Derived from Ultimate-II firmware source code at
https://github.com/GideonZ/1541ultimate/blob/master/software/io/network/network_target.cc

Copyright Richard Halkyard, 2019
*******************************************************************************/
#include <string.h>

#include "ucommand.h"
#include "unet.h"


/*
 * Open a TCP or UDP socket
 *
 * Arguments:
 *  kind:       0 for UDP, 1 for TCP
 *  port:       port to connect to
 *  hostname:   hostname to connect to
 *
 * Returns positive socket ID on success, or negative value on failure.
 */
int unet_open(uint8_t kind, uint16_t port, const char * hostname) {
    uint8_t sock;

    u2_start_cmd();
    U2_CMD = U2_TGT_NET;
    U2_CMD = kind ? NET_CMD_OPEN_TCP : NET_CMD_OPEN_UDP;
    U2_CMD = port & 0x00ff;
    U2_CMD = (port >> 8) & 0x00ff;
    u2_write(hostname, strlen(hostname));
    u2_finish_cmd();

    if (! (U2_CTL & U2_DATA_AVAIL)) {
        return -1;
    }

    sock = U2_DATA;

    u2_accept();

    return sock;
}


/*
 * Read data from a socket
 *
 * Arguments:
 *  sock:   socket ID
 *  buf:    buffer to read data into
 *  len:    maximum number of bytes to read
 *
 * Returns number of bytes read on success. 0 if socket has been closed, -1 if
 * no data available.
 */
int unet_read(uint8_t sock, uint8_t * buf, size_t len) {
    int sz;
    int res;

    u2_start_cmd();
    U2_CMD = U2_TGT_NET;
    U2_CMD = NET_CMD_READ_SOCKET;
    U2_CMD = sock;
    U2_CMD = len & 0x00ff;
    U2_CMD = (len >> 8) & 0x00ff;
    u2_finish_cmd();

    // First 2 bytes of data are the return code from the U2's recv() call
    sz = u2_read((uint8_t *) &res, 2, U2_READ_DATA);
    if (sz != 2) {
        // If we didn't get 2 bytes here, something is wrong; bail out.
        u2_accept();
        return sz;
    }

    // Now we read the socket data
    sz = u2_read(buf, res, U2_READ_DATA);

    u2_accept();

    return sz;
}

/*
 * Write data to a socket
 *
 * Arguments:
 *  sock:   socket ID
 *  buf:    buffer to write data from
 *  len:    maximum number of bytes to write
 *
 * Returns number of bytes written on success. -1 on error.
 */
int unet_write(uint8_t sock, uint8_t * buf, size_t len) {
    int res, written;

    /* Truncate data if necessary so that data+command fits in the U2 command
       buffer */
    if (len + 3 > U2_CMD_MAXLEN) {
        len = U2_CMD_MAXLEN - 3;
    }

    u2_start_cmd();
    U2_CMD = U2_TGT_NET;
    U2_CMD = NET_CMD_WRITE_SOCKET;
    U2_CMD = sock;
    u2_write(buf, len);
    u2_finish_cmd();

    res = u2_read((uint8_t *) &written, 2, U2_READ_DATA);

    u2_accept();

    if (res != 2) {
        return 0;
    }

    return written;
}


/*
 * Close a socket
 *
 * Arguments:
 *  sock: socket ID
 */
void unet_close(uint8_t sock) {
    u2_start_cmd();
    U2_CMD = U2_TGT_NET;
    U2_CMD = NET_CMD_CLOSE_SOCKET;
    U2_CMD = sock;
    u2_finish_cmd();
}
