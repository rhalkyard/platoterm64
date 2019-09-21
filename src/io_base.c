/**
 * PLATOTerm64 - A PLATO Terminal for the Commodore 64
 * Based on Steve Peltz's PAD
 * 
 * Author: Thomas Cherryhomes <thom.cherryhomes at gmail dot com>
 *
 * io.c - Input/output functions (serial/ethernet)
 */

#include <stdbool.h>
#include <serial.h>
#include <stdint.h>
#include <stdlib.h>
#include <peekpoke.h>
#include <stdio.h>
#include "io.h"
#include "protocol.h"
#include "config.h"
#include "prefs.h"

#define NULL 0

#define RECV_BUFSZ 384

uint8_t xoff_enabled;
uint8_t io_load_successful=false;

void io_open_base(void);
void io_main_base(void);
void io_done_base(void);
void io_connect_base(void);

void (*io_open)(void) = io_open_base;
void (*io_connect)(void) = io_connect_base;
void (*io_main)(void) = io_main_base;
void (*io_done)(void) = io_done_base;

uint8_t (*io_serial_buffer_size)(void);
void (*io_recv_serial_flow_off)(void);
void (*io_recv_serial_flow_on)(void);

static uint8_t ch=0;
static uint8_t io_res;
uint8_t recv_buffer[RECV_BUFSZ];
static uint16_t recv_buffer_size=0;
extern ConfigInfo config;

static struct ser_params params = {
  SER_BAUD_38400,
  SER_BITS_8,
  SER_STOP_1,
  SER_PAR_NONE,
  SER_HS_HW
};

/**
 * io_init() - Set-up the I/O
 */
void io_init(void)
{
  if (config.io == CONFIG_IO_U2ETH && PEEK(0xDF1D) != 0xc9) {
    config.io = CONFIG_IO_SERIAL;
  }

  prefs_clear();
  if (config.io == CONFIG_IO_SERIAL) {
    prefs_display("serial driver loaded.");
    io_res=ser_load_driver(io_ser_driver_name(config.driver_ser));

    if (io_res==SER_ERR_OK)
      io_load_successful=true;
    
    xoff_enabled=false;

    if (io_load_successful)
      {
        io_init_funcptrs();
        io_open();
        prefs_clear();
        prefs_display("serial driver opened.");
      }
    else
      {
        prefs_clear();
        sprintf(recv_buffer,"open err: %d",io_res);
        prefs_display(recv_buffer);
      }

  } else if (config.io == CONFIG_IO_U2ETH) {
    prefs_display("Using U2 ethernet for IO.");
    io_load_successful = true;
    io_init_funcptrs();
    io_open();
  } 
}

void io_connect_base(void) {

}

/**
 * io_open() - Open the device
 */
void io_open_base(void)
{
  params.baudrate = config.baud;
  
  io_res=ser_open(&params);
  
  if (io_res!=SER_ERR_OK)
    {
      prefs_clear();
      io_load_successful=false;
      prefs_display("error: could not open serial port.");
    }
}

/**
 * io_main() - The IO main loop
 */
void io_main_base(void)
{
  if (io_load_successful==false)
    return;
  
  // Drain primary serial FIFO as fast as possible.
  while (ser_get(&ch)!=SER_ERR_NO_DATA && recv_buffer_size < RECV_BUFSZ)
    {
      recv_buffer[recv_buffer_size++]=ch;
      if (xoff_enabled==false && FlowControl && recv_buffer_size>config.xoff_threshold)
        {
          io_recv_serial_flow_off();
        }
    }

  ShowPLATO(recv_buffer,recv_buffer_size);

if (xoff_enabled==true)
	{
		io_recv_serial_flow_on();
	}

  recv_buffer_size=0;
}

/**
 * io_done() - Called to close I/O
 */
void io_done_base(void)
{
  if (io_load_successful==false)
    return;
  
  ser_close();
  ser_uninstall();
}
 