/*******************************************************************************
Simple API for outbound TCP and UDP connections using the Ultimate II ethernet
interface.

Derived from Ultimate-II firmware source code at
https://github.com/GideonZ/1541ultimate/blob/master/software/io/network/network_target.cc

Copyright Richard Halkyard, 2019
*******************************************************************************/

#include <stddef.h>
#include <stdint.h>


#define U2_TGT_NET                  0x03

#define NET_CMD_IDENTIFY            0x01
#define NET_CMD_GET_INTERFACE_COUNT 0x02
#define NET_CMD_SET_INTERFACE       0x03
#define NET_CMD_GET_NETADDR         0x04
#define NET_CMD_GET_IPADDR          0x05
#define NET_CMD_SET_IPADDR          0x06
#define NET_CMD_OPEN_TCP	        0x07
#define NET_CMD_OPEN_UDP	        0x08
#define NET_CMD_CLOSE_SOCKET        0x09
#define NET_CMD_READ_SOCKET         0x10
#define NET_CMD_WRITE_SOCKET        0x11

int unet_open(uint8_t kind, uint16_t port, const char * hostname);
int unet_read(uint8_t sock, uint8_t * buf, size_t len);
int unet_write(uint8_t sock, uint8_t * buf, size_t len);
void unet_close(uint8_t sock);
