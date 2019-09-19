/*******************************************************************************
Low-level API for the 1541 Ultimate II command interface

Derived from Ultimate-II documentation at
https://1541u-documentation.readthedocs.io/en/latest/command%20interface.html

Copyright Richard Halkyard, 2019
*******************************************************************************/

#include "ucommand.h"

static char * stbuf = NULL;
static size_t stbufsz = 0;

/*
 * Return 1 if Ultimate-II command interface is present (reading the command
 * register at 0xDF1D returns 0xC9), 0 if not present.
 */
uint8_t u2_check(void) {
    if (U2_CMD == U2_DEVICEID) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Enable automatic reading of the status stream into a user-provided buffer.
 *
 * Arguments:
 *  buf: buffer to read status into
 *  sz:  size of buffer
 *
 * U2_STATUS_LEN, defined in ucommand.h, is the maximum possible size of status
 * data.
 */
void u2_stbuf_enable(char * buf, size_t sz) {
    stbuf = buf;
    stbufsz = sz;
}

/*
 * Prepare for a new command. Clear any error state, and abort any command
 * that is currently in progress
 */
void u2_start_cmd(void) {
    if (U2_CTL & U2_ERROR) {
        U2_CTL = U2_ERROR;
    }
    if (U2_CTL & U2_STATEMASK) {
        u2_abort();
    }
}

/*
 * Issue a command and wait for it to complete. If a status buffer was set up,
 * read status stream into buffer.
 *
 * Returns 0 if command was issued successfully, nonzero if an error occurred.
 */
uint8_t u2_finish_cmd(void) {
    if (U2_CTL & U2_ERROR) {
        return 1;
    } else {
        // Push command
        U2_CTL = U2_PUSH;
        while (U2_CTL & U2_PUSH) {}

        // Wait for command to complete
        while ((U2_CTL & U2_STATEMASK) == U2_STATE_BUSY) {}
    }

    // Read status stream if buffer has been set up.
    if (stbuf != NULL && stbufsz > 0) {
        u2_read(stbuf, stbufsz, U2_READ_STATUS);
    }

    return 0;
}

/*
 * Accept a data packet.
 */
void u2_accept(void) {
    uint8_t oldstatus = U2_CTL & U2_STATEMASK;

    // Set accept bit
    U2_CTL = U2_ACCEPT;
    while (U2_CTL & U2_ACCEPT) {}

    // If there's more data to follow, wait until it's ready.
    if (oldstatus == U2_STATE_DMORE) {
        while ((U2_CTL && U2_STATEMASK) == U2_STATE_BUSY) {}
    }
}

/*
 * Abort a command
 */
void u2_abort(void) {
    // Set abort bit and wait for state to return to idle
    U2_CTL = U2_ABORT;
    while (U2_CTL & U2_STATEMASK) {};
}

/*
 * Issue a command from a buffer.
 *
 * Arguments:
 *  buf: buffer containing command
 *  len: length of command
 *
 * Returns the number of bytes written on success, 0 on failure.
 */
size_t u2_command(const uint8_t * buf, size_t len) {
    size_t sz;

    u2_start_cmd();
    sz = u2_write(buf, len);

    if (u2_finish_cmd()) {
        u2_abort();
        return 0;
    }

    return sz;
}

/*
 * Write data to the command interface.
 *
 * Arguments:
 *  buf: buffer containing data
 *  len: length of data
 *
 * Returns the number of bytes written.
 */
size_t u2_write(const uint8_t * buf, size_t len) {
    size_t sz = 0;

    while (sz < len && !(U2_CTL & U2_ERROR)) {
        U2_CMD = buf[sz++];
    }

    return sz;
}

/*
 * Get one byte from the data stream, advancing to the next data packet if
 * necessary.
 *
 * Returns a byte, or -1 if no data is available.
 */
static int u2_getdata(void) {
    if (U2_CTL & U2_DATA_AVAIL) {
        return U2_DATA;
    } else if ((U2_CTL & U2_STATEMASK) == U2_STATE_DMORE) {
        // No data available in current packet, but state shows that another
        // packet is available. Accept() to move on to next packet, and return
        // the first byte.
        u2_accept();
        return U2_DATA;
    } else {
        return -1;
    }
}

/*
 * Get one byte from the status stream.
 *
 * Returns a byte, or -1 if no data is available.
 */
static int u2_getstatus(void) {
    if (U2_CTL & U2_STATUS_AVAIL) {
        return U2_STATUS;
    } else {
        return -1;
    }
}

/*
 * Read from data or status streams into a buffer.
 *
 * Arguments:
 *  buf:  buffer to read into
 *  len:  number of bytes to read
 *  what: U2_READ_DATA to read data stream, U2_READ_STATUS to read status.
 *
 * Returns number of bytes read, or -1 if `what` is invalid.
 */
size_t u2_read(uint8_t * buf, size_t len, uint8_t what) {
    size_t sz;
    int res;
    int (*get)(void);

    switch (what) {
        case U2_READ_DATA:
            get = u2_getdata;
            break;
        case U2_READ_STATUS:
            get = u2_getstatus;
            break;
        default:
            return -1;
    }

    for (sz = 0; sz < len; sz++) {
        res = get();
        if (res >= 0) {
            buf[sz] = get();
        } else {
            break;
        }
    }

    return sz;
}
