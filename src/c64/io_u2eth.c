/**
 * PLATOTerm64 - A PLATO Terminal for the Commodore 64
 * Based on Steve Peltz's PAD
 * 
 * io_u2eth.c - Input/output functions for 1541 Ultimate II ethernet
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


/* Buffer for user input at connection prompt */
char inputbuf[80];
uint8_t inputbufsz = 0;
uint8_t promptShown = 0;

/* Buffer for error strings returned on Ultimate-II's status stream */
char stbuf[256];

/**
 * io_send_byte_u2eth(b) - Send specified byte out
 */
void io_send_byte_u2eth(uint8_t b)
{
  if (sock < 0 && inputbufsz < 80) {
    // We're not connected, echo the character to the screen and store it in
    // the input buffer (if there's room)
    ShowPLATO(&b, 1);
    inputbuf[inputbufsz++] = b;
  } else if (sock >= 0) {
    // Otherwise, write the data out.
    unet_write(sock, &b, 1);
  }
}

/**
 * io_send_byte_u2eth(b) - Prepare device
 */
void io_open_u2eth(void) {
  // Set up buffer for error text
  u2_stbuf_enable(stbuf, 256);
}

/**
 * u2eth_show_connect_prompt(b) - Show prompt on greeting screen
 */
void u2eth_show_connect_prompt(void) {
  const char promptMsg[] = "Enter a host:port to connect to:\r\n";
  ShowPLATO((padByte *) promptMsg, strlen(promptMsg));
}

/**
 * u2eth_connect(b) - Parse input hostname and try to connect
 */
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

  // If port not specified, use 8005
  if (portstr == NULL) {
    port = 8005;
  } else {
    port = atoi(portstr);
  }

  ShowPLATO((padByte *) connectingMsg, strlen(connectingMsg));

  sock = unet_open(1, port, hostname);

  if (!(stbuf[0]=='0' && stbuf[1]=='0')) {
    // If we don't get a "00 OK" status, something went wrong; display the
    // error message
    ShowPLATO((padByte *) connErrMsg, strlen(connErrMsg));
    ShowPLATO(stbuf, strlen(stbuf));
    sock = -1;
  }
}

/**
 * io_main_u2eth(b) - Main IO loop
 */
void io_main_u2eth(void) {
  int sz;
    
  if (sock >= 0) {
    // While connected, read and display data. No need to bother with flow
    // control, since that's offloaded onto the U2's TCP stack
    sz = unet_read(sock, recv_buffer, 384);
    if (sz <= 0) {
      return;
    }

    ShowPLATO(recv_buffer, sz);
  } else if (inputbufsz > 0 && inputbuf[inputbufsz-1] == 0x0d) {
    // If not connected, and the last character entered was RETURN, try to
    // connect
    ShowPLATO("\r\n", 2);
    u2eth_connect();

    // Reset prompt flag, so if we fail to connect, we display the prompt again
    promptShown = 0; 
  } else if (promptShown == 0) {
    // Prompt has not been shown, show it
    u2eth_show_connect_prompt();
    promptShown = 1;
  }
}

/**
 * io_recv_serial_flow_off_u2eth() - Tell remote system to stop transmitting
 */
void io_recv_serial_flow_off_u2eth(void)
{
  io_send_byte_u2eth(0x13);
  xoff_enabled=true;
}

/**
 * io_recv_serial_flow_on_u2eth() - Tell remote system to resume transmitting
 */
void io_recv_serial_flow_on_u2eth(void)
{
  io_send_byte_u2eth(0x11);
  xoff_enabled=false;
}

/**
 * io_done_u2eth() - Close connection.
 */
void io_done_u2eth(void) {
  if (sock == -1)
    return;

  unet_close(sock);
  sock = -1;
}
