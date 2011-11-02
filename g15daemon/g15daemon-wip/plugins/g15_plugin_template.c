 /*
    This file is part of g15daemon.

    g15daemon is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g15daemon is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with g15daemon; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    (c) 2006-2007 Mike Lampard, Philip Lawatsch, and others
    
    $Revision: 347 $ -  $Date: 2007-12-19 21:09:02 -0800 (Wed, 19 Dec 2007) $ $Author: mlampard $
        
simple template plugin, replace the various functions with your own, and change the g15plugin_info struct below to suit,
   edit Makefile.am and compile.  Add salt and pepper to taste.  For a more advanced plugin that creates it's own lcd screens on-the-fly, 
   see the tcpserver plugin in this directory.
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <config.h>
#include <g15daemon.h>


static int *lcdclock(lcd_t *lcd)
{
    unsigned int col = 0;
    unsigned int len=0;
    int narrows=0;
    int totalwidth=0;
    char buf[10];
    
    time_t currtime = time(NULL);
    
        memset(lcd->buf,0,1024);
        memset(buf,0,10);
        strftime(buf,6,"%H:%M",localtime(&currtime));

        if(buf[0]==49) 
            narrows=1;

        len = strlen(buf); 

        if(narrows)
            totalwidth=(len*20)+(15);
        else
            totalwidth=len*20;

        for (col=0;col<len;col++) 
        {
            draw_bignum (lcd, (80-(totalwidth)/2)+col*20, 1,(80-(totalwidth)/2)+(col+1)*20, LCD_HEIGHT, BLACK, buf[col]);

        }
        lcd->ident = currtime+100;
    
    return G15_PLUGIN_OK;
}

static int myeventhandler(plugin_event_t *myevent) {
//    lcd_t *lcd = (lcd_t*) myevent->lcd;
    
    switch (myevent->event)
    {
        case G15_EVENT_KEYPRESS:
        printf("template plugin received keypress event : %i\n",myevent->value);
          break;
        case G15_EVENT_VISIBILITY_CHANGED:
        printf("template received new visibility status (%i)\n",myevent->value);
          break;
        default:
          break;
    }
    return G15_PLUGIN_OK;
}

/* completely uncessary function called when plugin is exiting */
static void *callmewhenimdone(lcd_t *lcd){
}

/* completely unnecessary initialisation function which could just as easily have been set to NULL in the g15plugin_info struct */
static void *myinithandler(lcd_t *lcd){
}

/* if no exitfunc or eventhandler, member should be NULL */
plugin_info_t g15plugin_info[] = {
    /* TYPE, name, initfunc, updatefreq, exitfunc, eventhandler, initfunc */
    {G15_PLUGIN_LCD_CLIENT, "template plugin clock", (void*)lcdclock, 500, (void*)callmewhenimdone, (void*)myeventhandler, (void*)myinithandler},
    {G15_PLUGIN_NONE,               ""          , NULL,     0,   NULL,            NULL,           NULL}
};
