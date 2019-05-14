/**
 * PLATOTerm64 - A PLATO Terminal for the Commodore 64
 * Based on Steve Peltz's PAD
 * 
 * Author: Thomas Cherryhomes <thom.cherryhomes at gmail dot com>
 *
 * touch.c - Touchscreen functions
 */

#include <stdbool.h>
#include <string.h>
#include <mouse.h>
#include <peekpoke.h>
#include "touch.h"

#ifdef __ATARI__
#include <atari.h>
#endif


static struct mouse_info mouse_data;
static uint8_t mouse_present=false;
static padBool touch_allowed=false;

/**
 * touch_init() - Set up touch screen
 */
void touch_init(void)
{
#ifndef __APPLE2__
  if (mouse_install(&mouse_def_callbacks,atrjoy_mou) == MOUSE_ERR_OK)
    {
      mouse_present=false;
      /* mouse_show(); */
#ifdef __ATARI__
      POKE(0xD000,0);
      POKE(0xD001,0);
      /* POKE(0xD002,0); */
      POKE(0xD003,0);
#endif /* __ATARI__ */
    }
#endif /* __APPLE2__ */
  /* mouse_present=false; */
  /* touch_allowed=false; */
  /* mouse_hide(); */
}

/**
 * touch_allow
 */
void touch_allow(padBool allow)
{
  if (touch_allowed==false && allow==true)
    {
      touch_allowed=true;
      mouse_show();
    }
  else if (touch_allowed==true && allow==false)
    {
      touch_allowed=false;
      mouse_hide();
    }
    
}

/**
 * touch_main - Process mouse events and turn into scaled touch events
 */
void touch_main(void)
{
#ifndef __APPLE2__
  uint8_t lastbuttons;
  padPt coord;

  if (mouse_present==false || touch_allowed==false)
    return;
  
  mouse_info(&mouse_data);
  
  if (mouse_data.buttons == lastbuttons)
    return; /* debounce */
  else if ((mouse_data.buttons & MOUSE_BTN_LEFT))
    {
      coord.x=mouse_data.pos.x;
      coord.y=mouse_data.pos.y;
      touch_translate(&coord);
      Touch(&coord);
    }
  lastbuttons = mouse_data.buttons;
#endif 
}

/**
 * touch_done() - Stop the mouse driver
 */
void touch_done(void)
{
#ifndef __APPLE2__
  if (mouse_present==true)
    mouse_uninstall();
#endif
}
