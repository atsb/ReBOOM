// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_system.c,v 1.15 1998/09/07 20:06:44 jim Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include <stdio.h>

#ifdef _WIN32 // proff: Includes for Windows
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef _MSC_VER
#include <conio.h>
#include <stdarg.h>
#endif
#endif //_WIN32

#ifdef UNIX
#include "SDL2/SDL.h"
#else
#include <SDL.h>
#endif

#include "i_system.h"
#include "i_sound.h"
#include "doomstat.h"
#include "m_misc.h"
#include "g_game.h"
#include "w_wad.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

#include "i_system.h"

int mousepresent;
int joystickpresent;                                         // phares 4/3/98

ticcmd_t *I_BaseTiccmd(void)
{
  static ticcmd_t emptycmd; // killough
  return &emptycmd;
}

#ifndef _WIN32
//
// I_SetJoystickDevice
//
// haleyjd
//
// haleyjd: SDL joystick support

// current device number -- saved in config file
int i_SDLJoystickNum = -1;
 
// pointer to current joystick device information
SDL_Joystick *sdlJoystick = NULL;

boolean I_SetJoystickDevice(int deviceNum)
{
   if(deviceNum >= SDL_NumJoysticks())
      return false;
   else
   {
      sdlJoystick = SDL_JoystickOpen(deviceNum);

      if(!sdlJoystick)
     return false;

      // check that the device has at least 2 axes and
      // 4 buttons
      if(SDL_JoystickNumAxes(sdlJoystick) < 2 ||
     SDL_JoystickNumButtons(sdlJoystick) < 4)
     return false;

      return true;
   }
}
#endif

void I_WaitVBL(int count)
{
#ifdef _WIN32 // proff: Added Sleep-function
    // proff: Changed time-calculation
    Sleep((count*500)/TICRATE);
#else
    SDL_Delay((count*500)/TICRATE);
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

// Most of the following has been rewritten by Lee Killough
//
// I_GetTime
//

static volatile int realtic;

#ifdef DJGPP
void I_timer(void)
{
  realtic++;
}
END_OF_FUNCTION(I_timer);

int  I_GetTime_RealTime (void)
{
  return realtic;
}
#else //DJGPP

#ifdef _WIN32
//
// I_GetTrueTime
// returns time in 1/70th second tics
//
int  I_GetTrueTime (void)
{
// proff 08/15/98: Added QueryPerfomanceCounter and changed the function a bit
  static int            first=1;
  int                   tm;
  static int            basetime;
#ifdef _MSC_VER
  static int            QPC_Available=0;
  static LARGE_INTEGER  QPC_Freq;
  LARGE_INTEGER         QPC_tm;
#endif

  if (first==1)
  {
#ifdef _MSC_VER
    if (QueryPerformanceFrequency(&QPC_Freq))
    {
      QPC_Available=1;
      QueryPerformanceCounter(&QPC_tm);
      basetime=(int)(QPC_tm.QuadPart*1000/QPC_Freq.QuadPart);
      lprintf (LO_INFO, "Using QueryPerformanceCounter\n");
    }
    else
#endif
    {
      basetime=timeGetTime();
      lprintf (LO_INFO, "Using timeGetTime\n");
    }
    first=0;
  }
#ifdef _MSC_VER
  if (QPC_Available)
        {
    QueryPerformanceCounter(&QPC_tm);
    tm=(int)(QPC_tm.QuadPart*1000/QPC_Freq.QuadPart);
        }
  else
#endif
  {
    tm = timeGetTime();
    }
  return (tm-basetime);
}
#endif

/* set_leds:
 *  Overrides the state of the keyboard LED indicators.
 *  Set to -1 to return to default behavior.
 */
static void set_leds(int leds)
{
    //Gibbon - stub, for historical purposes
}

//
// I_GetTime
// returns time in 1/70th second tics
//

static Uint32 basetime=0;

int  I_GetTime_RealTime (void)
{
#ifdef _WIN32 // proff: Added function for Windows
// proff 10/31/98: Moved the core of this function to I_GetTrueTime
  realtic = (I_GetTrueTime()*TICRATE)/1000;
  return realtic;
#else //_WIN32
  // haleyjd
  Uint32        ticks;
    
  // milliseconds since SDL initialization
  ticks = SDL_GetTicks();
    
  return ((ticks - basetime)*TICRATE)/1000;
#endif //_WIN32
}
#endif //DJGPP

// killough 4/13/98: Make clock rate adjustable by scale factor
#ifndef _MSC_VER // proff: using __int64 instead of long long, because it's not defined in Visual C
int realtic_clock_rate = 100;
static long long I_GetTime_Scale = 1<<24;
int I_GetTime_Scaled(void)
{
  return (long long) realtic * I_GetTime_Scale >> 24;
}
#else //_MSC_VER
int realtic_clock_rate = 100;
static __int64 I_GetTime_Scale = 1<<24;
int I_GetTime_Scaled(void)
{
// proff: Added typecast to avoid warning
// proff 10/31/98: Substituted realtic with I_GetTime_RealTime, because realtic
//                 is not updated through an interupt like in DOS
  return (int)((__int64) I_GetTime_RealTime() * I_GetTime_Scale >> 24);
}
#endif //_MSC_VER

static int  I_GetTime_FastDemo(void)
{
  static int fasttic;
  return fasttic++;
}

static int I_GetTime_Error()
{
  I_Error("Error: GetTime() used before initialization");
  return 0;
}

int (*I_GetTime)() = I_GetTime_Error;                           // killough

// killough 3/21/98: Add keyboard queue

#ifndef _WIN32 // proff: This is provided by Windows, so we don't need it
int mousepresent;
int joystickpresent;                                         // phares 4/3/98

static int orig_key_shifts;  // killough 3/6/98: original keyboard shift state
int key_shifts;
extern int autorun;          // Autorun state

static SDL_Keymod oldmod; // haleyjd: save old modifier key state
#endif //_WIN32

int leds_always_off;         // Tells it not to update LEDs

void I_Shutdown(void)
{
#ifndef _WIN32 // proff: Not needed in Windows
    SDL_SetModState(oldmod);
    
    // haleyjd 04/15/02: shutdown joystick
    if(joystickpresent && sdlJoystick && i_SDLJoystickNum >= 0)
    {
       if(SDL_JoystickGetAttached(sdlJoystick))
          SDL_JoystickClose(sdlJoystick);
       
       joystickpresent = false;
    }
    SDL_Quit();
#endif //_WIN32
}

void I_Init(void)
{
#ifndef _WIN32 // proff: Not needed in Windows
    extern int key_autorun;
    
    // init timer
    basetime = SDL_GetTicks();
#endif //_WIN32

  // killough 4/14/98: Adjustable speedup based on realtic_clock_rate
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
   else
    if (realtic_clock_rate != 100)
   {
#ifndef _MSC_VER // proff: using __int64 instead of long long, because it's not defined in Visual C
        I_GetTime_Scale = ((long long) realtic_clock_rate << 24) / 100;
#else //_MSC_VER
        I_GetTime_Scale = ((__int64) realtic_clock_rate << 24) / 100;
#endif //_MSC_VER
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

#ifndef _WIN32 // proff: Not needed in Windows

  // killough 3/6/98: save keyboard state, initialize shift state and LEDs:

  orig_key_shifts = key_shifts;  // save keyboard state

  key_shifts = 0;        // turn off all shifts by default
      
    SDL_Keymod   mod;
       
    oldmod = SDL_GetModState();
    switch(key_autorun)
    {
    case KEYD_CAPSLOCK:
       mod = KMOD_CAPS;
       break;
    case KEYD_NUMLOCK:
       mod = KMOD_NUM;
       break;
    case KEYD_SCROLLLOCK:
       mod = KMOD_MODE;
       break;
    default:
       mod = KMOD_NONE;
    }
    
    if(autorun)
       SDL_SetModState(mod);
    else
       SDL_SetModState(KMOD_NONE);
   
  // Either keep the keyboard LEDs off all the time, or update them
  // right now, and in the future, with respect to key_shifts flag.
  set_leds(leds_always_off ? 0 : -1);
  // killough 3/6/98: end of keyboard / autorun state changes

  // phares 4/3/98:
  // Init the joystick
  // For now, we'll require that joystick data is present in allegro.cfg.
  // The ASETUP program can be used to obtain the joystick data.
   
  // haleyjd
  if(i_SDLJoystickNum != -1)
  {
     joystickpresent = I_SetJoystickDevice(i_SDLJoystickNum);
  }
  else
  {
     joystickpresent = false;
  }

#endif //_WIN32
   atexit(I_Shutdown);
   
  { // killough 2/21/98: avoid sound initialization if no sound & no music
      extern boolean nomusicparm, nosfxparm;
    if (!(nomusicparm && nosfxparm))
	 I_InitSound();
   }
}

//
// I_Quit
//

static char errmsg[2048];    // buffer of error message -- killough

static int has_exited;

void I_Quit (void)
{
  has_exited=1;   /* Prevent infinitely recursive exits -- killough */

#ifdef _WIN32 // proff: Needed different order in Windows
    if (*errmsg)
  {
    //proff 9/17/98 use logical output routine
    lprintf (LO_ERROR, "%s\n", errmsg);
// proff 07/29/98: Changed MessageBox-Title to "PrBoom"
    MessageBox(NULL,errmsg,"BOOM",0);
  }
  else
    {
    if (demorecording)
      G_CheckDemoStatus();
    M_SaveDefaults ();
        I_EndDoom();
    // proff: Changed to I_WaitVBL and made longer
    I_WaitVBL(140);
    }
#else //_WIN32
   if (demorecording)
      G_CheckDemoStatus();
  M_SaveDefaults ();

  if (*errmsg)
    //jff 8/3/98 use logical output routine
    lprintf (LO_ERROR, "%s\n", errmsg);
  else
    I_EndDoom();
#endif //_WIN32
}

//
// I_Error
//

void I_Error(const char *error, ...) // killough 3/20/98: add const
{
  if (!*errmsg)   // ignore all but the first message -- killough
   {
      va_list argptr;
      va_start(argptr,error);
      vsprintf(errmsg,error,argptr);
      va_end(argptr);
   }
   
  if (!has_exited)    // If it hasn't exited yet, exit now -- killough
   {
      has_exited=1;   // Prevent infinitely recursive exits -- killough
      exit(-1);
   }
}


// killough 2/22/98: Add support for ENDBOOM, which is PC-specific
#ifdef _WIN32 // proff: Functions to access the console
extern HWND con_hWnd;

void textattr(byte a)
{
  int r,g,b,col;
  HDC conDC;

  conDC=GetDC(con_hWnd);
  r=0; g=0; b=0;
  if (a & FOREGROUND_INTENSITY) col=255;
  else col=128;
  if (a & FOREGROUND_RED) r=col;
  if (a & FOREGROUND_GREEN) g=col;
  if (a & FOREGROUND_BLUE) b=col;
  SetTextColor(conDC, PALETTERGB(r,g,b));
  r=0; g=0; b=0;
  if (a & BACKGROUND_INTENSITY) col=255;
  else col=128;
  if (a & BACKGROUND_RED) r=col;
  if (a & BACKGROUND_GREEN) g=col;
  if (a & BACKGROUND_BLUE) b=col;
 	SetBkColor(conDC, PALETTERGB(r,g,b));
  ReleaseDC(con_hWnd,conDC);
}

void I_EndDoom(void)
{
  int lump = W_CheckNumForName("ENDBOOM"); //jff 4/1/98 sign our work
  if (lump != -1)
    {
      const char (*endoom)[2] = W_CacheLumpNum(lump, PU_STATIC);
      int i, l = W_LumpLength(lump) / 2;
      for (i=0; i<l; i++)
        {
          textattr(endoom[i][1]);
          lprintf(LO_ALWAYS,"%c",endoom[i][0]);
        }
    }
}
#endif

//----------------------------------------------------------------------------
//
// $Log: i_system.c,v $
// Revision 1.15  1998/09/07  20:06:44  jim
// Added logical output routine
//
// Revision 1.14  1998/05/03  22:33:13  killough
// beautification
//
// Revision 1.13  1998/04/27  01:51:37  killough
// Increase errmsg size to 2048
//
// Revision 1.12  1998/04/14  08:13:39  killough
// Replace adaptive gametics with realtic_clock_rate
//
// Revision 1.11  1998/04/10  06:33:46  killough
// Add adaptive gametic timer
//
// Revision 1.10  1998/04/05  00:51:06  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.9  1998/04/02  05:02:31  jim
// Added ENDOOM, BOOM.TXT mods
//
// Revision 1.8  1998/03/23  03:16:13  killough
// Change to use interrupt-driver keyboard IO
//
// Revision 1.7  1998/03/18  16:17:32  jim
// Change to avoid Allegro key shift handling bug
//
// Revision 1.6  1998/03/09  07:12:21  killough
// Fix capslock bugs
//
// Revision 1.5  1998/03/03  00:21:41  jim
// Added predefined ENDBETA lump for beta test
//
// Revision 1.4  1998/03/02  11:31:14  killough
// Fix ENDOOM message handling
//
// Revision 1.3  1998/02/23  04:28:14  killough
// Add ENDOOM support, allow no sound FX at all
//
// Revision 1.2  1998/01/26  19:23:29  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
