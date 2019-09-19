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
#include <peekpoke.h>
#include "../config.h"
#include "../prefs.h"
#include "../protocol.h"

#include "unet.h"
#include "ucommand.h"

extern ConfigInfo config;

char connect_host[] = "irata.online";
const uint16_t connect_port = 8005;

extern uint8_t xoff_enabled;

extern uint8_t recv_buffer[];

char filename[80];
uint8_t n=0;
FILE * f;

int sock = -255;

char stbuf[256];

/**
 * io_send_byte(b) - Send specified byte out
 */
void io_send_byte_u2eth(uint8_t b)
{
  if (sock == -1)
    return;
  
  unet_write(sock, &b, 1);
}

void io_open_u2eth(void) {
}

void io_connect_u2eth(void) {
  u2_stbuf_enable(stbuf, 256);
  sock = unet_open(1, connect_port, connect_host);

  prefs_clear();
  prefs_display(stbuf);
}

void io_main_u2eth(void) {
  int sz;
    
  if (sock < 0)
    return;
  
  // Drain primary serial FIFO as fast as possible.
  sz = unet_read(sock, recv_buffer, 384);
  if (sz <= 0) {
    return;
  }

  ShowPLATO(recv_buffer, sz);

  sz = 0;
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
