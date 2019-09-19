#include "ucommand.h"
#include <peekpoke.h>

static char * stbuf = NULL;
static size_t stbufsz = 0;

void u2_stbuf_enable(char * buf, size_t sz) {
    stbuf = buf;
    stbufsz = sz;
}

void u2_start_cmd(void) {
    if (U2_CTL & U2_ERROR) {
        U2_CTL = U2_ERROR;
    }
    if (U2_CTL & U2_STATEMASK) {
        u2_abort();
    }
}

uint8_t u2_finish_cmd(void) {
    if (U2_CTL & U2_ERROR) {
        return 0;
    } else {
        U2_CTL = U2_PUSH;

        while (! (U2_CTL & U2_PUSH)) {}
        while ((U2_CTL & U2_STATEMASK) == U2_STATE_BUSY) {}
    }

    if (stbuf != NULL && stbufsz > 0) { 
        u2_read(stbuf, stbufsz, U2_READ_STATUS);
    }

    return U2_CTL;
}

void u2_accept(void) {
    uint8_t oldstatus = U2_CTL & U2_STATEMASK;

    U2_CTL = U2_ACCEPT;
    // while (!(U2_CTL & U2_ACCEPT)) {}
    if (oldstatus == U2_STATE_DMORE) {
        while ((U2_CTL && U2_STATEMASK) == U2_STATE_BUSY) {}
    }
}

void u2_abort(void) {
    U2_CTL = U2_ABORT;
    while (U2_CTL & U2_STATEMASK) {};
}

size_t u2_command(const uint8_t * buf, size_t len) {
    size_t sz;

    u2_start_cmd();
    sz = u2_write(buf, len);

    if (sz != len) {
        u2_abort();
    } else {
        u2_finish_cmd();
    }

    return sz;
}

size_t u2_write(const uint8_t * buf, size_t len) {
    size_t sz = 0;

    while (sz < len && !(U2_CTL & U2_ERROR)) {
        U2_CMD = buf[sz++];
    }

    return sz;
}

static int u2_getdata(void) {
    if (U2_CTL & U2_DATA_AVAIL) {
        return U2_DATA;
    } else if ((U2_CTL & U2_STATEMASK) == U2_STATE_DMORE) {
        u2_accept();
        return U2_DATA;
    } else {
        return -1;
    }
}

static int u2_getstatus(void) {
    if (U2_CTL & U2_STATUS_AVAIL) {
        return U2_STATUS;
    } else {
        return -1;
    }
}

size_t u2_read(uint8_t * buf, size_t len, uint8_t what) {
    size_t sz;
    int res;

    for (sz = 0; sz < len; sz++) {
        switch (what) {
            case U2_READ_DATA:
                res = u2_getdata();
                break;
            case U2_READ_STATUS:
                res = u2_getstatus();
                break;
            default:
                res = -1;
        }
        if (res >= 0) {
            buf[sz] = res;
        } else {
            break;
        }
    }

    return sz;
}
