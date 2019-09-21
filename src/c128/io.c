/**
 * PLATOTerm64 - A PLATO Terminal for the Commodore 64
 * Based on Steve Peltz's PAD
 * 
 * Author: Thomas Cherryhomes <thom.cherryhomes at gmail dot com>
 *
 * io.c - Input/output functions (serial/ethernet) (c64 specific)
 */

#include <cbm.h>
#include <c128.h>
#include <peekpoke.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <serial.h>
#include "../config.h"
#include "../io.h"
#include "io_u2eth.h"

extern uint8_t xoff_enabled;
extern ConfigInfo config;
extern uint8_t (*io_serial_buffer_size)(void);
extern void (*io_recv_serial_flow_off)(void);
extern void (*io_recv_serial_flow_on)(void);
extern uint8_t io_load_successful;
extern uint8_t already_started;

void io_recv_serial_flow_off_swiftlink(void);
void io_recv_serial_flow_on_swiftlink(void);
uint8_t io_serial_buffer_size_swiftlink(void);

/**
 * io_init_funcptrs() - Set up I/O function pointers
 */
void io_init_funcptrs(void)
{
  if (io_load_successful==false)
    return;
  
  if (config.io == CONFIG_IO_U2ETH)
    {
      // Override base implementations of io_open, io_main and io_done
      io_open = io_open_u2eth;
      io_main = io_main_u2eth;
      io_done = io_done_u2eth;

      io_recv_serial_flow_off = io_recv_serial_flow_off_u2eth;
      io_recv_serial_flow_on = io_recv_serial_flow_on_u2eth;
    }
  else
    {
      if (config.driver_ser==CONFIG_SERIAL_DRIVER_SWIFTLINK)
        {
          io_serial_buffer_size=io_serial_buffer_size_swiftlink;
          io_recv_serial_flow_off=io_recv_serial_flow_off_swiftlink;
          io_recv_serial_flow_on=io_recv_serial_flow_on_swiftlink;
        }
    }
}

/**
 * io_send_byte(b) - Send specified byte out
 */
void io_send_byte(uint8_t b)
{
  if (io_load_successful==false || already_started==false)
    return;
  if (config.io == CONFIG_IO_SERIAL) {
    ser_put(b);
  } else if (config.io == CONFIG_IO_U2ETH) {
    io_send_byte_u2eth(b);
  }
}

/**
 * Return the serial buffer size
 */
uint8_t io_serial_buffer_size_swiftlink(void)
{
  if (io_load_successful==false)
    return 0;

  return PEEK(0xF9)-PEEK(0xF8);
}

/**
 * io_recv_serial_flow_off() - Tell modem to stop receiving.
 */
void io_recv_serial_flow_off_swiftlink(void)
{
  if (io_load_successful==false)
    return;

  io_send_byte(0x13);
  xoff_enabled=true;
}

/**
 * io_recv_serial_flow_on() - Tell modem to stop receiving.
 */
void io_recv_serial_flow_on_swiftlink(void)
{
  if (io_load_successful==false)
    return;

  io_send_byte(0x11);
  xoff_enabled=false;
}

/**
 * io_ser_driver_name() - return serial driver name given constant
 */
const char* io_ser_driver_name(unsigned char driver)
{
  switch (driver)
    {
    case CONFIG_SERIAL_DRIVER_SWIFTLINK:
      return "ser-swlink";
    }
}
