/*
 * Copyright (c) 2002-2004, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution. 
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the Contiki desktop environment
 *
 * $Id: ssfire.c,v 1.1 2005/05/12 21:12:43 oliverschmidt Exp $
 *
 */

#include <stdlib.h>

#include "ctk.h"
#include "ctk-draw.h"
#include "ctk-mouse.h"
#include "ek.h"
#include "loader.h"


/*static DISPATCHER_SIGHANDLER(ssfire_sighandler, s, data);
static void ssfire_idle(void);
static struct dispatcher_proc p =
  {DISPATCHER_PROC("Fire screensaver", ssfire_idle,
		   ssfire_sighandler,
		   NULL)};
		   static ek_id_t id;*/

EK_EVENTHANDLER(ssfire_eventhandler, ev, data);
EK_POLLHANDLER(ssfire_pollhandler);
EK_PROCESS(p, "Fire screensaver", EK_PRIO_LOWEST,
	   ssfire_eventhandler, ssfire_pollhandler, NULL);
static ek_id_t id = EK_ID_NONE;

static unsigned char flames[8*17];

static const unsigned char flamecolors[16] =
  {0x00, 0x00, 0x00, 0x11, 0x99, 0xDD, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
   
static unsigned char *flameptr;
static unsigned char x, y;


/*-----------------------------------------------------------------------------------*/
LOADER_INIT_FUNC(ssfire_init, arg)
{
  arg_free(arg);
  
  if(id == EK_ID_NONE) {
    id = ek_start(&p);
  }
}
/*-----------------------------------------------------------------------------------*/
static void
fire_quit(void)
{
  *(char *)0xC051 = 0;

  ek_exit();
  id = EK_ID_NONE;
  LOADER_UNLOAD();
}
/*-----------------------------------------------------------------------------------*/
static void
fire_init(void)
{
  *(char *)0xC056 = 0;
  *(char *)0xC054 = 0;
  *(char *)0xC052 = 0;
  *(char *)0xC050 = 0;

  for(y = 0; y < 24; ++y) {
    gotoy(y);
    for(x = 0; x < 40; ++x) {
      (*(unsigned char **)0x28)[x] = 0x00;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
EK_EVENTHANDLER(ssfire_eventhandler, ev, data)
{
  EK_EVENTHANDLER_ARGS(ev, data);
  
  if(ev == EK_EVENT_INIT) {
    ctk_mode_set(CTK_MODE_SCREENSAVER);
    ctk_mouse_hide();
    fire_init();
  } else if(ev == ctk_signal_screensaver_stop ||
	    ev == EK_EVENT_REQUEST_EXIT) {
    fire_quit();
    ctk_draw_init();
    ctk_desktop_redraw(NULL);
  }
}
/*-----------------------------------------------------------------------------------*/
#pragma optimize(push, off)
static void
fire_burn(void)
{
  /* Calculate new flames. */
  asm("ldy #$00");
loop1:
  asm("lda %v+7,y", flames);
  asm("clc");
  asm("adc %v+8,y", flames);
  asm("adc %v+9,y", flames);
  asm("adc %v+16,y", flames);
  asm("lsr");
  asm("lsr");
  asm("sta %v,y", flames);
  asm("iny");
  asm("cpy #(8*15)");
  asm("bne %g", loop1);

  /* Fill last line with pseudo-random data. */
  asm("ldy #$05");
loop2:
  asm("jsr %v", rand);
  asm("and #$0F");
  asm("sta %v+8*15+1,y", flames);
  asm("dey");
  asm("bpl %g", loop2);
}
#pragma optimize(pop)
/*-----------------------------------------------------------------------------------*/
EK_POLLHANDLER(ssfire_pollhandler)
{
  if(ctk_mode_get() == CTK_MODE_SCREENSAVER) {

    fire_burn();
  
    /* Display flames on screen. */  
    flameptr = flames;
    for(y = 9; y < 24; ++y) {
      gotoy(y);
      for(x = 0; x < 8; ++x) {
	(*(unsigned char **)0x28)[x   ] =
	(*(unsigned char **)0x28)[x+32] = flamecolors[flameptr[x]];
      }
      flameptr += 8;
    }
  }
}
/*-----------------------------------------------------------------------------------*/

