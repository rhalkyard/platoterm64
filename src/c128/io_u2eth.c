/**
 * PLATOTerm64 - A PLATO Terminal for the Commodore 64
 * Based on Steve Peltz's PAD
 * 
 * Author: Thomas Cherryhomes <thom.cherryhomes at gmail dot com>
 *
 * io.c - Input/output functions (serial/ethernet) (c64 specific)
 */


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "../config.h"
#include "../prefs.h"
#include "../protocol.h"
#include "../plato_key.h"

#include "unet.h"
#include "ucommand.h"

extern ConfigInfo config;
extern uint8_t xoff_enabled;
extern uint8_t recv_buffer[];

int sock = -255;

char inputbuf[80];
uint8_t inputbufsz = 0;
uint8_t promptShown = 0;

char stbuf[256];

/**
 * io_send_byte(b) - Send specified byte out
 */
void io_send_byte_u2eth(uint8_t b)
{
  if (sock < 0 && inputbufsz < 80) {
    ShowPLATO(&b, 1);
    inputbuf[inputbufsz++] = b;
  } else if (sock >= 0) {
    unet_write(sock, &b, 1);
  }
}

void io_open_u2eth(void) {
  u2_stbuf_enable(stbuf, 256);
}

void u2eth_show_connect_prompt(void) {
  const char promptMsg[] = "Enter a host:port to connect to:\r\n";
  ShowPLATO((padByte *) promptMsg, strlen(promptMsg));
}

void u2eth_connect(void) {
  const char connectingMsg[] = "Connecting... ";
  const char connErrMsg[] = "Unable to connect: ";

  char * hostname;
  char * portstr;
  uint16_t port;

  inputbuf[inputbufsz-1] = '\0';
  inputbufsz = 0;

  hostname = strtok(inputbuf, ":");
  portstr = strtok(NULL, ":");

  if (portstr == NULL) {
    port = 8005;
  } else {
    port = atoi(portstr);
  }

  ShowPLATO((padByte *) connectingMsg, strlen(connectingMsg));

  sock = unet_open(1, port, hostname);

  if (!(stbuf[0]=='0' && stbuf[1]=='0')) {
    ShowPLATO((padByte *) connErrMsg, strlen(connErrMsg));
    ShowPLATO(stbuf, strlen(stbuf));
    sock = -1;
  }
}

void io_main_u2eth(void) {
  int sz;
    
  if (sock >= 0) {
    // Drain primary serial FIFO as fast as possible.
    sz = unet_read(sock, recv_buffer, 384);
    if (sz <= 0) {
      return;
    }

    ShowPLATO(recv_buffer, sz);
  } else if (inputbufsz > 0 && inputbuf[inputbufsz-1] == 0x0d) {
    ShowPLATO("\r\n", 2);
    u2eth_connect();
    promptShown = 0;
  } else if (promptShown == 0) {
    u2eth_show_connect_prompt();
    promptShown = 1;
  }
}

void io_recv_serial_flow_off_u2eth(void)
{
  io_send_byte_u2eth(0x13);
  xoff_enabled=true;
}

/**
 * io_recv_serial_flow_on() - Tell modem to stop receiving.
 */
void io_recv_serial_flow_on_u2eth(void)
{
  io_send_byte_u2eth(0x11);
  xoff_enabled=false;
}

void io_done_u2eth(void) {
  if (sock == -1)
    return;

  unet_close(sock);
  sock = -1;
}
