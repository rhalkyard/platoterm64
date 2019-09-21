#include <stddef.h>
#include <stdint.h>

#define U2_CTL          (*(uint8_t*)0xdf1c)
#define U2_CMD          (*(uint8_t*)0xdf1d)
#define U2_DATA         (*(uint8_t*)0xdf1e)
#define U2_STATUS       (*(uint8_t*)0xdf1f)

#define U2_DEVICEID     0xc9

#define U2_PUSH         (1 << 0)
#define U2_ACCEPT       (1 << 1)
#define U2_ABORT        (1 << 2)
#define U2_ERROR        (1 << 3)

#define U2_STATEMASK    ((1 << 4) | (1 << 5))
#define U2_STATE_IDLE   0
#define U2_STATE_BUSY   (1 << 4)
#define U2_STATE_DLAST  (1 << 5)
#define U2_STATE_DMORE  ((1 << 4) | (1 << 5))

#define U2_STATUS_AVAIL (1 << 6)
#define U2_DATA_AVAIL   (1 << 7)

#define U2_CMD_MAXLEN   896
#define U2_STATUS_LEN   256

enum u2_read_source {
    U2_READ_DATA = 0,
    U2_READ_STATUS = 1
};

extern int unet_errno;

uint8_t u2_check(void);
void u2_stbuf_enable(char * buf, size_t sz);
void u2_start_cmd(void);
uint8_t u2_finish_cmd(void);
void u2_accept(void);
void u2_abort(void);
size_t u2_command(const uint8_t * buf, size_t len);
size_t u2_write(const uint8_t * buf, size_t len);
size_t u2_read(uint8_t * buf, size_t len, uint8_t what);
