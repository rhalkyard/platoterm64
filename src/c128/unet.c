#include <string.h>

#include "ucommand.h"
#include "unet.h"
#include <peekpoke.h>


int unet_errno;

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

size_t unet_read(uint8_t sock, uint8_t * buf, size_t len) {
    size_t sz;
    uint8_t res[2];

    u2_start_cmd();
    U2_CMD = U2_TGT_NET;
    U2_CMD = NET_CMD_READ_SOCKET;
    U2_CMD = sock;
    U2_CMD = len & 0x00ff;
    U2_CMD = (len >> 8) & 0x00ff;
    u2_finish_cmd();

    sz = u2_read(res, 2, U2_READ_DATA);
    if (sz != 2) {
        unet_errno = *(int *) res;
        u2_accept();
        return 0;
    }

    sz = u2_read(buf, len, U2_READ_DATA);

    u2_accept();

    return sz;
}

size_t unet_write(uint8_t sock, uint8_t * buf, size_t len) {
    size_t res, written;

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
        unet_errno = res;
        return 0;
    }
    
    return written;
}

void unet_close(uint8_t sock) {
    u2_start_cmd();
    U2_CMD = U2_TGT_NET;
    U2_CMD = NET_CMD_CLOSE_SOCKET;
    U2_CMD = sock;
    u2_finish_cmd();
}
