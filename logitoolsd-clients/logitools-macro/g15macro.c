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

        (c) 2006-2009 Mike Lampard

        $Revision: 537 $ -  $Date: 2011-01-10 01:35:55 -0800 (Mon, 10 Jan 2011) $ $Author: steelside $

        This daemon listens on localhost port 15550 for client connections,
        and arbitrates LCD display.  Allows for multiple simultaneous clients.
        Client screens can be cycled through by pressing the 'L1' key.

        This is a macro recorder and playback utility for the G15 and g15daemon.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pwd.h>
#include <pthread.h>
#include <sys/time.h>
#include <config.h>
#include <X11/Xlib.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#ifdef HAVE_X11_EXTENSIONS_XTEST_H
#include <X11/extensions/XTest.h>
#endif
#include <X11/XF86keysym.h>

#include <g15daemon_client.h>
#include <libg15.h>
#include <libg15render.h>
#include "config.h"
#include "g15macro_small.xbm"

#define  XK_MISCELLANY
#define XK_LATIN1
#define XK_LATIN2
#include <X11/keysymdef.h>

#include "g15macro.h"

int leaving = 0;
int display_timeout=500;
int debug = 0;

int mmedia_codes[6] = {164, 162, 144, 153, 174, 176};
const long mmedia_defaults[] = {
	XF86XK_AudioStop,
 XF86XK_AudioPlay,
 XF86XK_AudioPrev,
 XF86XK_AudioNext,
 XF86XK_AudioLowerVolume,
 XF86XK_AudioRaiseVolume
};

int gkeycodes[18] = { 177,152,190,208,129,130,231,209,210,136,220,143,246,251,137,138,133,183 };
const char *gkeystring[19] = { "G1","G2","G3","G4","G5","G6","G7","G8","G9","G10","G11","G12","G13","G14","G15","G16","G17","G18","Unknown"};
/* because this is an X11 client, we can work around the kernel limitations on key numbers */
const long gkeydefaults[54] = {
	/* M1 palette */
	XF86XK_Launch4,
 XF86XK_Launch5,
 XF86XK_Launch6,
 XF86XK_Launch7,
 XF86XK_Launch8,
 XF86XK_Launch9,
 XF86XK_LaunchA,
 XF86XK_LaunchB,
 XF86XK_LaunchC,
 XF86XK_LaunchD,
 XF86XK_LaunchE,
 XF86XK_LaunchF,
 XF86XK_iTouch,
 XF86XK_Calculater,
 XF86XK_Support,
 XF86XK_Word,
 XF86XK_Messenger,
 XF86XK_WebCam,
 /* M2 palette */
 XK_F13,
 XK_F14,
 XK_F15,
 XK_F16,
 XK_F17,
 XK_F18,
 XK_F19,
 XK_F20,
 XK_F21,
 XK_F22,
 XK_F23,
 XK_F24,
 XK_F25,
 XK_F26,
 XK_F27,
 XK_F28,
 XK_F29,
 XK_F30,
 /* M3 palette */
 XK_Tcedilla,
 XK_racute,
 XK_abreve,
 XK_lacute,
 XK_cacute,
 XK_ccaron,
 XK_eogonek,
 XK_ecaron,
 XK_dcaron,
 XK_dstroke,
 XK_nacute,
 XK_ncaron,
 XK_odoubleacute,
 XK_udoubleacute,
 XK_rcaron,
 XK_uring,
 XK_scaron,
 XK_abovedot
};


struct current_recording {
	keypress_t recorded_keypress[MAX_KEYSTEPS];
}current_recording;

unsigned int rec_index=0;

pthread_t Xkeys;
pthread_t Lkeys;

int map_gkey(keystate){
    int retval = -1;
    switch(keystate){
        case G15_KEY_G1:  retval = 0;   break;
        case G15_KEY_G2:  retval = 1;   break;
        case G15_KEY_G3:  retval = 2;   break;
        case G15_KEY_G4:  retval = 3;   break;
        case G15_KEY_G5:  retval = 4;   break;
        case G15_KEY_G6:  retval = 5;   break;
        case G15_KEY_G7:  retval = 6;   break;
        case G15_KEY_G8:  retval = 7;   break;
        case G15_KEY_G9:  retval = 8;   break;
        case G15_KEY_G10: retval = 9;   break;
        case G15_KEY_G11: retval = 10;   break;
        case G15_KEY_G12: retval = 11;   break;
        case G15_KEY_G13: retval = 12;   break;
        case G15_KEY_G14: retval = 13;   break;
        case G15_KEY_G15: retval = 14;   break;
        case G15_KEY_G16: retval = 15;   break;
        case G15_KEY_G17: retval = 16;   break;
        case G15_KEY_G18: retval = 17;   break;
    }
    return retval;
}

/* debugging wrapper */
static int g15macro_log (const char *fmt, ...) {

    if (debug) {
        printf("G15Macro: ");
        va_list argp;
        va_start (argp, fmt);
        vprintf(fmt,argp);
        va_end (argp);
        fflush(stdout);
    }

    return 0;
}

//Stolen from http://stackoverflow.com/questions/122616/painless-way-to-trim-leading-trailing-whitespace-in-c/122974#122974
char *trim(char *str)
{
	size_t len = 0;
	char *frontp = str - 1;
	char *endp = NULL;

	if( str == NULL )
		return NULL;

	if( str[0] == '\0' )
		return str;

	len = strlen(str);
	endp = str + len;

    /* Move the front and back pointers to address
	* the first non-whitespace characters from
	* each end.
	*/
	while( isspace(*(++frontp)) );
	while( isspace(*(--endp)) && endp != frontp );

	if( str + len - 1 != endp )
		*(endp + 1) = '\0';
	else if( frontp != str &&  endp == frontp )
		*str = '\0';

    /* Shift the string so that it starts at str so
	* that if it's dynamically allocated, we can
	* still free it on the returned pointer.  Note
	* the reuse of endp to mean the front of the
	* string buffer now.
	*/
	endp = str;
	if( frontp != str )
	{
		while( *frontp ) *endp++ = *frontp++;
		*endp = '\0';
	}


	return str;
}

// Trims a string to a specific length
// Caller needs to free the data.
char* stringTrim(const char* source, const unsigned int len)
{
	char* dest = malloc(len);
	memset(dest,0,len);
	strncpy(dest,source,len-1);

	return dest;
}

//TODO: Put into libg15render instead
void drawXBM(g15canvas* canvas, unsigned char* data, int width, int height ,int pos_x, int pos_y)
{
	int y = 0;
	int z = 0;
	unsigned char byte;
	int bytes_per_row = ceil((double) width / 8);

	int bits_left = width;
	int current_bit = 0;

	for(y = 0; y < height; y ++)
	{
		bits_left = width;
		for(z=0;z < bytes_per_row; z++)
		{
			byte = data[(y * bytes_per_row) + z];
			current_bit = 0;
			while(current_bit < 8)
			{
				if(bits_left > 0)
				{
					if((byte >> current_bit) & 1) g15r_setPixel(canvas, (current_bit + (z*8) + pos_x),y + pos_y,G15_COLOR_BLACK);
					bits_left--;
				}
				current_bit++;
			}
		}
	}
}

void cleanup()
{
	int i;
	if(recording){
		recording = 0;
		XUngrabKeyboard(dpy,CurrentTime);
	}

	memset(configpath,0,sizeof(configpath));
	strcpy(configpath,configDir);
	strncat(configpath,configs[currConfig]->configfile,sizeof(configpath)-strlen(configpath));
	save_macros(configpath);

	for (i = 0; i < MAX_CONFIGS; ++i)
	{
		if(configs[i])
		{
			if (configs[i]->configfile)
			{
				free(configs[i]->configfile);
				configs[i]->configfile = NULL;
			}
			free(configs[i]);
		}
		configs[i] = NULL;
	}

	g15_send_cmd (g15screen_fd,G15DAEMON_MKEYLEDS,0);

	pthread_join(Xkeys,NULL);
	pthread_join(Lkeys,NULL);
	pthread_mutex_destroy(&x11mutex);
	pthread_mutex_destroy(&config_mutex);
	pthread_mutex_destroy(&gui_select);


	/* revert the keymap to g15daemon default on exit */
	change_keymap(0);
	close(g15screen_fd);

	emptyMstates(1);
	free(mstates[0]);
	free(mstates[1]);
	free(mstates[2]);
}

void cleanupChildren (int signal_number)
{
	int status;
	wait (&status);
}

int runFile(char* file)
{
	pid_t pid;

	g15macro_log("Attempting to run %s\n",file);
	/* Attempt to fork and check for errors */
	if( (pid=fork()) == -1)
	{
		g15macro_log("Fork error\n");
		return 1;
	}

	if(pid)
	{
		/* A positive (non-negative) PID indicates the parent process */
		g15macro_log("Successfully forked. Child is %i\n", pid);
	}
	else
	{
		/* A zero PID indicates that this is the child process */
		/* Replace the child fork with a new process */
		if(execlp(file,file,NULL) == -1)
		{
			g15macro_log("Unable to execute %s\n", file);
			cleanup();
			exit(1);
		}
	}


	return 0;
}


void renderHelp()
{
	// Draws the helpbox in the bottom right corner.
	g15r_drawLine(canvas, G15_LCD_WIDTH-37, 16, G15_LCD_WIDTH, 16, G15_COLOR_BLACK);
	g15r_drawLine(canvas, G15_LCD_WIDTH-37, 16, G15_LCD_WIDTH-37, G15_LCD_HEIGHT, G15_COLOR_BLACK);
	g15r_renderString (canvas, (unsigned char *)"1:Default\0", 3, G15_TEXT_SMALL, G15_LCD_WIDTH-35, 0);
	g15r_renderString (canvas, (unsigned char *)"2:     Up\0", 4, G15_TEXT_SMALL, G15_LCD_WIDTH-35, 0);
	g15r_renderString (canvas, (unsigned char *)"3:   Down\0", 5, G15_TEXT_SMALL, G15_LCD_WIDTH-35, 0);
	g15r_renderString (canvas, (unsigned char *)"4:     OK\0", 6, G15_TEXT_SMALL, G15_LCD_WIDTH-35, 0);
}

void renderSelectionList()
{
	// Draws the three presets in the list (Selected-1,Selected,Selected+1)
	// Find config id to render
	pthread_mutex_lock(&gui_select);
	/*static*/ int tmpRenderConfID = 0;

	// Render first entry
	if (gui_selectConfig > 0)
		tmpRenderConfID = gui_selectConfig-1;
	else
		tmpRenderConfID = numConfigs;

	char* renderLine = stringTrim((char*)getConfigName(tmpRenderConfID),25);
	g15r_renderString(canvas, (unsigned char*)renderLine, 0, G15_TEXT_MED, 1, 17);
	free(renderLine);
	renderLine = NULL;


	// Render middle entry available for selection
	renderLine = stringTrim((char*)getConfigName(gui_selectConfig),25);

	g15r_renderString(canvas, (unsigned char*)renderLine, 0, G15_TEXT_MED, 1, 26);
	free(renderLine);
	renderLine = NULL;

	// Render third (last) entry
	if (gui_selectConfig < numConfigs)
		tmpRenderConfID = gui_selectConfig+1;
	else
		tmpRenderConfID = 0;
	pthread_mutex_unlock(&gui_select);

	renderLine = stringTrim((char*)getConfigName(tmpRenderConfID),25);
	g15r_renderString(canvas, (unsigned char*)renderLine, 0, G15_TEXT_MED, 1, 35);
	free(renderLine);
	renderLine = NULL;

	// Make middle look selected by inverting colours
	g15r_pixelReverseFill(canvas, 0, 24, 121, 33, 0,G15_COLOR_BLACK);

	g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
	pthread_mutex_lock(&gui_select);
	gui_oldConfig = gui_selectConfig;
	pthread_mutex_unlock(&gui_select);
}

void renderFull()
{
	char currPreset[1024];

	g15macro_log("Redrawing whole screen.\n");

	g15r_clearScreen(canvas,G15_COLOR_WHITE);
	drawXBM(canvas, (unsigned char*)g15macro_small_bits, g15macro_small_width, g15macro_small_height, 0, 0); // Logo
	renderHelp(); // Help box to the right
	// Draw indicator of currently selected preset
	memset(currPreset,0,sizeof(currPreset));
	snprintf(currPreset,1024,"Current:%s",getConfigName(currConfig));
	g15r_drawLine(canvas, 50, 2, 50, 11, G15_COLOR_BLACK); // Line separating logo from Current: <config>
	g15r_renderString (canvas, (unsigned char *)currPreset, 0, G15_TEXT_MED, 53, 4); // Current: <config>

	// Draw selection list
	renderSelectionList();
}

void renderSelection()
{
	g15macro_log("Redrawing only selection box.\n");
	//Clear only the selection box, since that's the only thing updated
	g15r_pixelBox(canvas,0,15,G15_LCD_WIDTH-38,G15_LCD_HEIGHT,G15_COLOR_WHITE,0,1);

	// Draw selection list
	renderSelectionList();
}

void gui_selectChange(int change)
{
	pthread_mutex_lock(&gui_select);

	if (change == 0)
	{
		gui_selectConfig = 0;
		pthread_mutex_unlock(&gui_select);
		return;
	}

	if (gui_selectConfig == 0 && change < 0)
		gui_selectConfig = numConfigs;
	else
	{
		gui_selectConfig += change;
		if (gui_selectConfig > numConfigs)
			gui_selectConfig = 0;
		else if (gui_selectConfig < 0)
			gui_selectConfig = numConfigs;

	}

	pthread_mutex_unlock(&gui_select);
}
void emptyMstates(int purge)
{
	int m = 0;
	int g = 0;
	for (m = 0; m < 3; ++m)
	{
		for (g = 0; g < 18; ++g)
		{
			if (mstates[m]->gkeys[g].execFile && purge)
			{
				free(mstates[m]->gkeys[g].execFile);
			}
			mstates[m]->gkeys[g].execFile = NULL;
		}
	}
}
//TODO: Make it so that if logitech ever makes fewer or more m states that it could iterate.
void cleanMstates(int purge)
{
	pthread_mutex_lock(&config_mutex);

	emptyMstates(purge);

	//TODO: Reduce memory usage?
	//printf("Cleaning mstates. Size of mstates[0]->gkeys = %i\n",sizeof(mstates[0]->gkeys));
	//Cleaning mstates. Size of mstates[0]->gkeys = 516240
	//That's alot of memory for an empty set of gkeys. And about 1.5 megabyte of memory for all 3.
	memset(mstates[0]->gkeys,0,sizeof(mstates[0]->gkeys));
	memset(mstates[1]->gkeys,0,sizeof(mstates[1]->gkeys));
	memset(mstates[2]->gkeys,0,sizeof(mstates[2]->gkeys));
	pthread_mutex_unlock(&config_mutex);
}

int calc_mkey_offset() {
	int mkey_offset=0;
	switch(mkey_state){
		case 0:
			mkey_offset = 0;
			break;
		case 1:
			mkey_offset = 18;
			break;
		case 2:
			mkey_offset = 36;
			break;
		default:
			mkey_offset=0;
	}
	return mkey_offset;
}

void fake_keyevent(int keycode,int keydown,unsigned long modifiers){
  if(have_xtest && !recording) {
    #ifdef HAVE_X11_EXTENSIONS_XTEST_H
    pthread_mutex_lock(&x11mutex);
       XTestFakeKeyEvent(dpy, keycode,keydown, CurrentTime);
       XSync(dpy,False);
    pthread_mutex_unlock(&x11mutex);
    usleep(1500);
     #endif
  } else {
    XKeyEvent event;
    Window current_focus;
    int dummy = 0;
    int key = 0;

    pthread_mutex_lock(&x11mutex);
      XGetInputFocus(dpy,&current_focus, &dummy);
      key = XKeycodeToKeysym(dpy,keycode,0);
      if(keydown)
        event.type=KeyPress;
      else
        event.type=KeyRelease;

      event.keycode = keycode;
      event.serial = 0;
      event.send_event = False;
      event.display = dpy;
      event.x = event.y = event.x_root = event.y_root = 0;
      event.time = CurrentTime;
      event.same_screen = True;
      event.subwindow = None;
      event.window = current_focus;
      event.root = root_win;
      event.state = modifiers;
      XSendEvent(dpy,current_focus,False,0,(XEvent*)&event);
      XSync(dpy,False);
    pthread_mutex_unlock(&x11mutex);
  }
}

void record_cleanup(){
    //Do not send double mled status changes, will disrupt the LCD for some reason
	//(shifts 10-20 pixels to the right each time) Bug in g15daemon/libg15 i suspect.
    memset(recstring,0,strlen((char*)recstring));
    rec_index = 0;
    recording = 0;
    pthread_mutex_lock(&x11mutex);
    XUngrabKeyboard(dpy,CurrentTime);
    XFlush(dpy);
    pthread_mutex_unlock(&x11mutex);
}

void record_cancel(){
	g15r_clearScreen(canvas,G15_COLOR_WHITE);
    g15r_renderString (canvas, (unsigned char *)"Recording", 0, G15_TEXT_LARGE, 80-((strlen("Recording")/2)*8), 4);
    g15r_renderString (canvas, (unsigned char *)"Canceled", 0, G15_TEXT_LARGE, 80-((strlen("Canceled")/2)*8), 18);
    g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
    record_cleanup();
}

void record_complete(unsigned long keystate)
{
    char tmpstr[1024];
    int gkey = map_gkey(keystate);

    pthread_mutex_lock(&config_mutex);

    if(!rec_index) // nothing recorded - delete prior recording
        memset(mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress,0,sizeof(keysequence_t));
    else
        memcpy(mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress, &current_recording, sizeof(keysequence_t));

    mstates[mkey_state]->gkeys[gkey].keysequence.record_steps=rec_index;
    pthread_mutex_unlock(&config_mutex);

	g15r_clearScreen(canvas,G15_COLOR_WHITE);
    if(rec_index){
        strcpy(tmpstr,"For key ");
        strcat(tmpstr,gkeystring[map_gkey(keystate)]);
        g15macro_log("Recording Complete %s\n",tmpstr);
        g15r_renderString (canvas, (unsigned char *)"Recording", 0, G15_TEXT_LARGE, 80-((strlen("Recording")/2)*8), 4);
        g15r_renderString (canvas, (unsigned char *)"Complete", 0, G15_TEXT_LARGE, 80-((strlen("Complete")/2)*8), 18);

    }else{
        strcpy(tmpstr,"From Key ");
		printf("%lu\n",keystate);
        strcat(tmpstr,gkeystring[map_gkey(keystate)]);
        g15macro_log("Macro deleted %s\n",tmpstr);
        g15r_renderString (canvas, (unsigned char *)"Macro", 0, G15_TEXT_LARGE, 80-((strlen("Macro")/2)*8), 4);
        g15r_renderString (canvas, (unsigned char *)"Deleted", 0, G15_TEXT_LARGE, 80-((strlen("Deleted")/2)*8), 18);
    }
    g15r_renderString (canvas, (unsigned char *)tmpstr, 0, G15_TEXT_LARGE, 80-((strlen(tmpstr)/2)*8), 32);

    g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);

    record_cleanup();
	g15_send_cmd (g15screen_fd,G15DAEMON_MKEYLEDS,mled_state);
    save_macros(configpath);
}


void macro_playback(unsigned long keystate)
{
    int i = 0;
    KeySym key;
    int keyevent;
    int gkey = map_gkey(keystate);
    if(gkey<0)
      return;

    /* if no macro has been recorded for this key,
	and no program set to execute,
	send the g15daemon default keycode */
    if(mstates[mkey_state]->gkeys[gkey].keysequence.record_steps==0 &&
	  mstates[mkey_state]->gkeys[gkey].execFile == NULL){
        int mkey_offset=0;

        mkey_offset = calc_mkey_offset();

        pthread_mutex_lock(&x11mutex);
        keyevent=XKeysymToKeycode(dpy, gkeydefaults[gkey+mkey_offset]);
        pthread_mutex_unlock(&x11mutex);

        fake_keyevent(keyevent,1,None);
        fake_keyevent(keyevent,0,None);
        g15macro_log("Key: \t%s\n",XKeysymToString(gkeydefaults[gkey+mkey_offset]));
        return;
    }
    g15macro_log("Macro Playback: for key %s\n",gkeystring[gkey]);
    pthread_mutex_lock(&config_mutex);
	if (mstates[mkey_state]->gkeys[gkey].execFile)
		runFile(mstates[mkey_state]->gkeys[gkey].execFile);
    for(i=0;i<mstates[mkey_state]->gkeys[gkey].keysequence.record_steps;i++){

        fake_keyevent(mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress[i].keycode,
                          mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress[i].pressed,
                          mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress[i].modifiers);

        pthread_mutex_lock(&x11mutex);
        key = XKeycodeToKeysym(dpy,mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress[i].keycode,0);
        pthread_mutex_unlock(&x11mutex);
        g15macro_log("\t%s %s\n",XKeysymToString(key),mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress[i].pressed?"Down":"Up");

        switch (key) {
            case XK_Control_L:
            case XK_Control_R:
            case XK_Meta_L:
            case XK_Meta_R:
            case XK_Alt_L:
            case XK_Alt_R:
            case XK_Super_L:
            case XK_Super_R:
            case XK_Hyper_L:
            case XK_Hyper_R:
             usleep(mstates[mkey_state]->gkeys[gkey].keysequence.recorded_keypress[i].time_ms*1000);
              break;
            default:
             usleep(1000);
        }
    }
    pthread_mutex_unlock(&config_mutex);
    g15macro_log("Macro Playback Complete\n");
}

int identify_configver(char *filename)
{
	FILE *f;
	char tmpstring[1024];

	unsigned int configver = 0;

	f=fopen(filename,"r");

	do
	{
		memset(tmpstring,0,1024);
		fgets(tmpstring,1024,f);

		// We ignore parsing comments
		// but next time this file is saved (like changing macro), they will be lost.
		if(tmpstring[0]=='#')
			continue;

		if (!configver && strncmp(tmpstring,"G15Macro config version",23) == 0)
		{
			sscanf(tmpstring,"G15Macro config version %i\n",&configver);
			if (G15MACRO_CONF_VER >= configver)
			{
				printf("Using config version %i. Highest supported is %i\n",configver,G15MACRO_CONF_VER);
			}
			else
			{
				printf("Config file is version %i. I support up to %i. Exiting.\n",configver,G15MACRO_CONF_VER);
				fclose(f);
				cleanup();
				exit(1);
			}
		}

	}while(!feof(f));

	fclose(f);

	return configver;
}

void restore_v1_config(char *filename)
{
	FILE *f;
	char tmpstring[1024];
	unsigned int key=0;
	unsigned int mkey=0;
	unsigned int i=0;
	unsigned int keycode;


	f=fopen(filename,"r");
	printf("Restoring macros from %s\n",filename);

	do
	{
		memset(tmpstring,0,1024);
		fgets(tmpstring,1024,f);

		// We ignore parsing comments
		// but next time this file is saved (like changing macro), they will be lost.
		if(tmpstring[0]=='#')
			continue;

		if(tmpstring[0]=='C'){
			sscanf(tmpstring,"Codes for MKey %i\n",&mkey);
			mkey--;
			i=0;
		}
		if(tmpstring[0]=='K'){
			sscanf(tmpstring,"Key G%i:",&key);
			key--;
			i=0;
		}
		if(tmpstring[0]=='\t'){
			char codestr[64];
			char pressed[20];
			unsigned int modifiers = 0;
			sscanf(tmpstring,"\t%s %s %i\n",(char*)&codestr,(char*)&pressed,&modifiers);
			keycode = XKeysymToKeycode(dpy,XStringToKeysym(codestr));
			mstates[mkey]->gkeys[key].keysequence.recorded_keypress[i].keycode = keycode;
			mstates[mkey]->gkeys[key].keysequence.recorded_keypress[i].pressed = strncmp(pressed,"Up",2)?1:0;
			mstates[mkey]->gkeys[key].keysequence.recorded_keypress[i].modifiers = modifiers;
			mstates[mkey]->gkeys[key].keysequence.record_steps=++i;
		}
	}while(!feof(f));

	fclose(f);
}
void restore_v2_config(char *filename)
{
	FILE *f;
	char tmpstring[1024];
	unsigned int key=0;
	unsigned int mkey=0;
	unsigned int i=0;
	unsigned int keycode;

	f=fopen(filename,"r");
	printf("Restoring macros from version 2 config: %s\n",filename);

	do
	{
		memset(tmpstring,0,1024);
		fgets(tmpstring,1024,f);

		// We ignore parsing comments
		// but next time this file is saved (like changing macro), they will be lost.
		if(tmpstring[0]=='#')
			continue;


		if(tmpstring[0]=='C'){
			sscanf(tmpstring,"Codes for MKey %i\n",&mkey);
			mkey--;
			i=0;
		}
		if(tmpstring[0]=='K'){
			sscanf(tmpstring,"Key G%i:",&key);
			key--;
			i=0;
		}
		if(tmpstring[0]=='\t')
		{
			char* substring = &tmpstring[1];
			if (strncmp(substring,"keypress",8) == 0)
			{
				char codestr[64];
				char pressed[20];
				unsigned int modifiers = 0;
				sscanf(substring,"keypress %s %s %i\n",(char*)&codestr,(char*)&pressed,&modifiers);
				keycode = XKeysymToKeycode(dpy,XStringToKeysym(codestr));
				mstates[mkey]->gkeys[key].keysequence.recorded_keypress[i].keycode = keycode;
				mstates[mkey]->gkeys[key].keysequence.recorded_keypress[i].pressed = strncmp(pressed,"Up",2)?1:0;
				mstates[mkey]->gkeys[key].keysequence.recorded_keypress[i].modifiers = modifiers;
				mstates[mkey]->gkeys[key].keysequence.record_steps=++i;
			}
			else if (strncmp(substring,"run",3) == 0)
			{
				char file[1024];
				memset(&file,0,sizeof(file));
// 				printf("Supposed to run from following line: %s",substring);
				sscanf(substring,"run %s\n",(char*)&file);
// 				printf("filename: %s\n",file);
				if (mstates[mkey]->gkeys[key].execFile)
				{
					printf("But key already has %s as file, not changing.\n",mstates[mkey]->gkeys[key].execFile);
					continue;
				}
				mstates[mkey]->gkeys[key].execFile = malloc(strlen(file)+1);
				memset(mstates[mkey]->gkeys[key].execFile,0,strlen(file)+1);
				strcpy(mstates[mkey]->gkeys[key].execFile,file);
// 				printf("Stored filename %s\n",mstates[mkey]->gkeys[key].execFile);
			}
		}
	}while(!feof(f));

	fclose(f);
}

void restore_config(char *filename)
{
	pthread_mutex_lock(&config_mutex);
	unsigned int configver = 0;
	configver = identify_configver(filename);

	if (configver == 0) // Original configuration
	{
		restore_v1_config(filename);
	}
	// Skipping V1 as i would call the original one for that.
	else if (configver == 2)
	{
		restore_v2_config(filename);
	}

	pthread_mutex_unlock(&config_mutex);
}

void loadMultiConfig()
{
	FILE *f;
	FILE *fCheck;
	char configPath[1024];
	char buf[1024]; // Max name length = 256
	char cfgName[256]; // Basically buf with \n stripped.
// 	unsigned int numConfigs = 0;
	int i = 0;

	// Initialize configs array of structs
	for (i = 0; i < MAX_CONFIGS; ++i)
	{
		configs[i] = NULL;
	}

	configs[0] = malloc(sizeof(configs_t));

	configs[0]->configfile = malloc(128);
	memset(configs[0]->configfile,0,sizeof(configs[0]->configfile));
	strcpy(configs[0]->configfile, "g15macro.conf");
	configs[0]->confver = 0;
	currConfig = 0;

	printf("default: %s\n",configs[0]->configfile);
	strncpy(configPath,configDir,sizeof(configPath));
	strncat(configPath,"multipleConfigs.cfg",sizeof(configPath)-strlen(configPath));

	f = fopen(configPath,"r");
	// File not created yet
	if (!f)
	{
		f = fopen(configPath,"w");
		fprintf(f,"#Place the name of the different configs here, 1 on each line.\n");
		fprintf(f,"#They will be represented by the filename inside g15macro. Max length = 255 characters.\n");
		fprintf(f,"#Comments need to be on their own line, and prepended by #, like these rows..\n");
		fprintf(f,"#At most you can have 32 different files. Ask on the forums if you absolutely need more.\n");
		fprintf(f,"#You do not need to include the default g15macro.cfg file, it will always be included, and referenced as Default on the lcd.\n");
		fprintf(f,"#Entries over roughly 15 characters will be cutoff in the Current: field. Entries over 25 characters will be cutoff in the selection display.\n");
		fclose(f);

		return;
	}
// 	pthread_mutex_lock(&config_mutex);

	while (!feof(f))
	{
		memset(buf,0,sizeof(buf));
		fgets(buf,sizeof(buf),f);

		// Ignore comments and blanklines
		if (buf[0] == '#' || strlen(buf) == 0)
			continue;


		// +1 cause this is going to be numConfigs+1 config number :) Must check if it will fit or not.
		// Can not increase numConfigs here, incase we break-out and don't have it in place - segfault.
		if(numConfigs+1 >= MAX_CONFIGS)
		{
			printf("Max of %i config files exceeded. Ignoring rest.\n",MAX_CONFIGS);
			break;
		}

		if (strlen(buf) > 255)
		{
			printf("Too long line found when reading in configs. Offending config name is %s\n",buf);
			continue;
		}
		i = strcspn(buf,"\n"); // strcspn returns "the length of the initial segment of buf which consists entirely of characters not in reject."
		memset(cfgName,0,sizeof(cfgName));
		strncpy(cfgName,buf,i);
		trim(cfgName);

		// Check if file exists
		strncpy(configPath,configDir,sizeof(configPath));
		strncat(configPath,cfgName,sizeof(configPath)-strlen(configPath));
		fCheck = fopen(configPath,"r");
		if (!fCheck)
		{
			printf("*** Unable to open '%s' - no such file or directory. Use\ntouch '%s'\n to create it.\n",configPath,configPath);
			continue;
		}
		fclose(fCheck);

		++numConfigs;

		// Add it to list of availible configurations
		g15macro_log("Adding Config %i with length %i - name %s\n",numConfigs,(int)strlen(cfgName),cfgName);
		configs[numConfigs] = (configs_t*)malloc(sizeof(configs_t));
		configs[numConfigs]->configfile = malloc(strlen(cfgName)+1); //+1 for null termination
		memset(configs[numConfigs]->configfile,0,strlen(cfgName)+1/*sizeof(configs[numConfigs]->configfile)*/);
		strcpy(configs[numConfigs]->configfile,cfgName);
		configs[numConfigs]->confver = 0;

	}
	fclose(f);
// 	pthread_mutex_unlock(&config_mutex);
}

void change_keymap(int offset){
    int i=0,j=0;
	//TODO: If they ever make a new G15keyboard(or this is reused for G19), change.
	int keys = G15Version == 0 ? 18 : 6;
    pthread_mutex_lock(&x11mutex);
    for(i=offset;i<offset + keys;i++,j++)
    {
      KeySym newmap[1];
      newmap[0]=gkeydefaults[i];
      XChangeKeyboardMapping (dpy, gkeycodes[j], 1, newmap, 1);
    }
    XFlush(dpy);
    pthread_mutex_unlock(&x11mutex);

}

/* ensure that the multimedia keys are configured */
void configure_mmediakeys(){
	KeySym newmap[1];
	int i=0;
	pthread_mutex_lock(&x11mutex);
	for(i=0;i<6;i++){
	newmap[0]=mmedia_defaults[i];
		XChangeKeyboardMapping (dpy, mmedia_codes[i], 1, newmap, 1);
	}
	XFlush(dpy);
	pthread_mutex_unlock(&x11mutex);
}

void handle_mkey_switch(unsigned int mkey) {
    int mkey_offset = 0;
    switch(mkey) {
      case G15_KEY_M1:
        mled_state=G15_LED_M1;
        mkey_state=0;
        break;
      case G15_KEY_M2:
        mled_state=G15_LED_M2;
        mkey_state=1;
        break;
      case G15_KEY_M3:
        mled_state=G15_LED_M3;
        mkey_state=2;
        break;
    }
    mkey_offset = calc_mkey_offset();
    if(recording)
      record_cancel();
    g15_send_cmd (g15screen_fd,G15DAEMON_MKEYLEDS,mled_state);
    change_keymap(mkey_offset);
}

void *Lkeys_thread() {
    unsigned long keystate = 0;
    struct pollfd fds;
    char ver[5];
    int foo = 0;
    float g15v;

	memset(ver,0x0,sizeof(ver));
	strncpy(ver,G15DAEMON_VERSION,3);
    sscanf(ver,"%f",&g15v);

    g15macro_log("Using version %.2f as keypress protocol\n",g15v);

    while(!leaving){

        /* g15daemon series 1.2 need key request packets */
        if((g15v*10)<=18) {
            keystate = g15_send_cmd (g15screen_fd, G15DAEMON_GET_KEYSTATE, foo);
        } else {
            fds.fd = g15screen_fd;
            fds.events = POLLIN;
            fds.revents=0;
            keystate=0;
            if ((poll(&fds, 1, 1000)) > 0) {
                read (g15screen_fd, &keystate, sizeof (keystate));
            }
        }

        if (keystate!=0)
        {
            g15macro_log("Received Keystate == %lu\n",keystate);

            switch (keystate)
            {
				case G15_KEY_L2:
				{
					int fg_check = g15_send_cmd (g15screen_fd, G15DAEMON_IS_FOREGROUND, foo);
					if(!fg_check)
						break;

					// Go to default/g15macro.conf = id =0
					gui_selectChange(0);
					break;
				}
				case G15_KEY_L3:
				{
					int fg_check = g15_send_cmd (g15screen_fd, G15DAEMON_IS_FOREGROUND, foo);
					if(!fg_check)
						break;

					// Scroll up
					gui_selectChange(-1);
					break;
				}
				case G15_KEY_L4:
				{
					int fg_check = g15_send_cmd (g15screen_fd, G15DAEMON_IS_FOREGROUND, foo);
					if(!fg_check)
						break;

					// Scroll down
					gui_selectChange(+1);
					break;
				}
				case G15_KEY_L5:
				{
					int fg_check = g15_send_cmd (g15screen_fd, G15DAEMON_IS_FOREGROUND, foo);
					if(!fg_check)
						break;

					// Change to selected

					char newConfig[1024];
					memset(newConfig,0,sizeof(newConfig));
					strcpy(newConfig,configDir);
					strncat(newConfig,configs[currConfig]->configfile,sizeof(newConfig)-strlen(newConfig));
					// Actually not the newConfig, it's the old, but didn't come up with a good name.
					save_macros(newConfig);

					// Purge all old data
					cleanMstates(1);

					// Now load the new config
					currConfig = gui_selectConfig;
					memset(newConfig,0,sizeof(newConfig));
					strcpy(newConfig,configDir);
					strncat(newConfig,configs[currConfig]->configfile,sizeof(newConfig)-strlen(newConfig));

					restore_config(newConfig);


					// Set the configpath to reflect the change
					memset(configpath,0,sizeof(configpath));
					strcpy(configpath,configDir);
					strncat(configpath,configs[currConfig]->configfile,sizeof(configpath)-strlen(configpath));
					gui_oldConfig = MAX_CONFIGS+1;

					break;
				}
                case G15_KEY_MR: {
					g15macro_log("Key pressed is MR\n");
                    if(!recording) {
                      if(0==g15_send_cmd (g15screen_fd, G15DAEMON_IS_FOREGROUND, foo)){
                        usleep(1000);
                        g15_send_cmd (g15screen_fd, G15DAEMON_SWITCH_PRIORITIES, foo);
                        g15macro_log("Switching to LCD foreground\n");
                      }
                      usleep(1000);
                      g15_send_cmd (g15screen_fd,G15DAEMON_MKEYLEDS, G15_LED_MR | mled_state);
                      g15r_initCanvas (canvas);
                      g15r_renderString (canvas, (unsigned char *)"Recording", 0, G15_TEXT_LARGE, 80-((strlen("Recording")/2)*8), 1);
                      g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
                      g15macro_log("Recording Enabled\n");
                      recording = 1;
                      pthread_mutex_lock(&x11mutex);
                      XGrabKeyboard(dpy, root_win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
                      pthread_mutex_unlock(&x11mutex);
                      memset(&current_recording,0,sizeof(current_recording));
                    } else {
                      record_cancel();
					  g15_send_cmd (g15screen_fd,G15DAEMON_MKEYLEDS,mled_state);
                    }
                    break;
                  }
                case G15_KEY_M1:
                    handle_mkey_switch(G15_KEY_M1);
					g15macro_log("Key pressed is M1\n");
                    break;
                case G15_KEY_M2:
                    handle_mkey_switch(G15_KEY_M2);
					g15macro_log("Key pressed is M2\n");
                    break;
                case G15_KEY_M3:
                    handle_mkey_switch(G15_KEY_M3);
					g15macro_log("Key pressed is M3\n");
                    break;
                default:
                    if(keystate >=G15_KEY_G1 && keystate <=G15_KEY_G18){
                        if(recording==1){
                            record_complete(keystate);
                        } else {
                            macro_playback(keystate);
                        }
                    }
                    break;
            }
            keystate = 0;
        }
    }
    return NULL;
}

unsigned int g15daemon_gettime_ms(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (tv.tv_sec*1000+tv.tv_usec/1000);
}

void xkey_handler(XEvent *event) {
	static unsigned long lasttime;
	unsigned char keytext[256];
	unsigned int keycode = event->xkey.keycode;
	int press = True;

	if(event->type==KeyRelease){ // we only do keyreleases for some keys
	pthread_mutex_lock(&x11mutex);
		KeySym key = XKeycodeToKeysym(dpy, keycode, 0);
	pthread_mutex_unlock(&x11mutex);
		switch (key) {
			case XK_Shift_L:
			case XK_Shift_R:
			case XK_Control_L:
			case XK_Control_R:
			case XK_Caps_Lock:
			case XK_Shift_Lock:
			case XK_Meta_L:
			case XK_Meta_R:
			case XK_Alt_L:
			case XK_Alt_R:
			case XK_Super_L:
			case XK_Super_R:
			case XK_Hyper_L:
			case XK_Hyper_R:
			default:
				press = False;
		}
	}
	if(recording){
		current_recording.recorded_keypress[rec_index].keycode = keycode;
		current_recording.recorded_keypress[rec_index].pressed = press;
		current_recording.recorded_keypress[rec_index].modifiers = event->xkey.state;
		if(rec_index==0)
			current_recording.recorded_keypress[rec_index].time_ms=0;
		else
			current_recording.recorded_keypress[rec_index].time_ms=g15daemon_gettime_ms() - lasttime;
		if(rec_index < MAX_KEYSTEPS)
		{
			rec_index++;
			/* now the default stuff */
			pthread_mutex_lock(&x11mutex);
			XUngrabKeyboard(dpy,CurrentTime);
			pthread_mutex_unlock(&x11mutex);

			fake_keyevent(keycode,press,event->xkey.state);

			pthread_mutex_lock(&x11mutex);
			XGrabKeyboard(dpy, root_win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
			XFlush(dpy);
			strcpy((char*)keytext,XKeysymToString(XKeycodeToKeysym(dpy, keycode, 0)));
			pthread_mutex_unlock(&x11mutex);
			if(0==strcmp((char*)keytext,"space"))
				strcpy((char*)keytext," ");
			if(0==strcmp((char*)keytext,"period"))
				strcpy((char*)keytext,".");
			if(press==True){
			strcat((char*)recstring,(char*)keytext);
			g15macro_log("Adding %s to Macro\n",keytext);
			g15r_renderString (canvas, (unsigned char *)recstring, 0, G15_TEXT_MED, 80-((strlen((char*)recstring)/2)*5), 22);
			g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
			}
		}
		else
		{
			pthread_mutex_lock(&x11mutex);
			XUngrabKeyboard(dpy,CurrentTime);
			pthread_mutex_unlock(&x11mutex);
			recording = 0;
			rec_index = 0;
			printf("Saving macro\n");
			save_macros(configpath);
		}

    }
	else
		rec_index=0;

	lasttime = g15daemon_gettime_ms();
}

static void* xevent_thread()
{
    XEvent event;
    long event_mask = KeyPressMask|KeyReleaseMask|FocusChangeMask|SubstructureNotifyMask;
    int retval=0;
    pthread_mutex_lock(&x11mutex);
    XSelectInput(dpy, root_win, event_mask);
    pthread_mutex_unlock(&x11mutex);
    while(!leaving){
        pthread_mutex_lock(&x11mutex);
        memset(&event,0,sizeof(XEvent));
        retval = XCheckMaskEvent(dpy, event_mask, &event);
        pthread_mutex_unlock(&x11mutex);
        if(retval == True){
            switch(event.type) {
                case KeyPress:
                    xkey_handler(&event);
                    break;
                case KeyRelease:
                    xkey_handler(&event);
                    break;
                case FocusIn:
                case FocusOut:
                case EnterNotify:
                case LeaveNotify:
                case MapNotify:
                case UnmapNotify:
                case MapRequest:
                case ConfigureNotify:
                case CreateNotify:
                case DestroyNotify:
                    break;
                case ReparentNotify: {
                        if(recording) {
                            pthread_mutex_lock(&x11mutex);
                            XGrabKeyboard(dpy, root_win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
                            XFlush(dpy);
                            pthread_mutex_unlock(&x11mutex);
                        }
                        break;
                    }
                default:
                    g15macro_log("Unhandled event (%i) received\n",event.type);
            }
        }else
        usleep(25000);
    }
    return NULL;
}

// TODO: handle errors
int myx_error_handler(Display *dpy, XErrorEvent *err){
    return 0;
}

void g15macro_sighandler(int sig) {
	switch(sig){
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
		case SIGPIPE:
		case SIGHUP:
			printf("Received signal %i\n",sig);
			leaving = 1;
			break;
	}
}

void helptext() {
  printf("G15Macro %s\n",PACKAGE_VERSION);
  printf("\n--user (-u) \"username\" run as user \"username\"\n");
  printf("--dump (-d) dump current configuration to stdout\n");
  printf("--debug (-g) print debugging information\n");
  printf("--version (-v) print version and exit\n");
  printf("--keysonly (-k) configure multimedia and extra keys then exit\n");
  printf("--g15version2 (-2) configure for G15v2 (6 G-Keys)\n");
  printf("--help (-h) this help text\n\n");
}

void mainLoop()
{
	do
	{
		if(display_timeout >= 0)
			--display_timeout;

		if(recording)
		{
			was_recording = 1;
			display_timeout=500;
		}

		if(display_timeout<=0 && (was_recording || gui_oldConfig != gui_selectConfig))
		{
			g15macro_log("Display timeout is 0, and we were recording OR selected a new config.\n");
			if (was_recording || gui_oldConfig == MAX_CONFIGS+1)
			{
				renderFull();
			}
			else
			{
				// Only need to update selection,
				renderSelection();
			}
			was_recording = 0;

			usleep(500*1000);
		}
		usleep(1000);

	} while( !leaving);
	// Told to exit.
	g15macro_log("Leaving mainloop\n");
}



int main(int argc, char **argv)
{
	// init vars
	have_xtest = False;
	numConfigs = 0;
	currConfig = 0;
	gui_selectConfig = 0;
	G15Version = 0;
	config_fd = 0;
	mled_state = G15_LED_M1;
	mkey_state = 0;
	recording = 0;
	// this is for keeping track of when to redraw
	gui_oldConfig = MAX_CONFIGS+1; // To make sure it will be redrawn at first
	was_recording = 1;

#ifdef USE_XTEST
    int xtest_major_version = 0;
    int xtest_minor_version = 0;
#endif
    struct sigaction new_action;
    int dummy=0,i=0;
    unsigned char user[256];
    struct passwd *username;
    char splashpath[1024];
    unsigned int dump = 0;
    unsigned int keysonly = 0;
    FILE *config;
    unsigned int convert = 0;

	memset(configDir,0,sizeof(configDir));
	strncpy(configDir,getenv("HOME"),1024);
	strncat(configDir,"/.g15macro/",1024-strlen(configDir));

    strncpy(configpath,getenv("HOME"),1024);

    memset(user,0,256);
    for(i=0;i<argc;i++){
        if (!strncmp(argv[i], "-u",2) || !strncmp(argv[i], "--user",6)) {
           if(argv[i+1]!=NULL){
             strncpy((char*)user,argv[i+1],128);
             i++;
           }
        }

        if (!strncmp(argv[i], "-d",2) || !strncmp(argv[i], "--dump",6)) {
          dump = 1;
        }

        if (!strncmp(argv[i], "-h",2) || !strncmp(argv[i], "--help",6)) {
          helptext();
          exit(0);
        }

        if (!strncmp(argv[i], "-k",2) || !strncmp(argv[i], "--keysonly",10)) {
          keysonly = 1;
        }

        if (!strncmp(argv[i], "-g",2) || !strncmp(argv[i], "--debug",7)) {
          printf("Debugging Enabled\n");
          debug = 1;
        }

        if (!strncmp(argv[i], "-v",2) || !strncmp(argv[i], "--version",9)) {
          printf("G15Macro version %s\n\n",PACKAGE_VERSION);
          exit(0);
        }

		if (!strncmp(argv[i], "-2",2) || !strncmp(argv[i], "--g15version2",13))
		{
			G15Version = 1; // See declaration for info
		}

    }

    if(strlen((char*)user)){
      username = getpwnam((char*)user);
        if (username==NULL) {
            username = getpwuid(geteuid());
            printf("BEWARE: running as effective uid %i\n",username->pw_uid);
        }
        else {
           if(0==setuid(username->pw_uid)) {
             setgid(username->pw_gid);
             strncpy(configpath,username->pw_dir,1024);
			 strncpy(configDir,username->pw_dir,1024);
             printf("running as user %s\n",username->pw_name);
           }
           else
             printf("Unable to run as user \"%s\" - you dont have permissions for that.\nRunning as \"%s\"\n",username->pw_name,getenv("USER"));
        }
		printf("BEWARE: this program will run files WITHOUT dropping any kind of privilegies.\n");
    }

    canvas = (g15canvas *) malloc (sizeof (g15canvas));

    if (canvas != NULL) {
        g15r_initCanvas(canvas);
    } else {
        printf("Unable to initialise the libg15render canvas\nExiting\n");
        return 1;
    }

    do {
      dpy = XOpenDisplay(getenv("DISPLAY"));
      if (!dpy) {
        printf("Unable to open display %s - retrying\n",getenv("DISPLAY"));
        sleep(2);
        }
    }while(!dpy);

    /* completely ignore errors and carry on */
    XSetErrorHandler(myx_error_handler);

	// Get keycodes for all keys
	strcpy(GKeyCodeCfg,configDir);
	strncat(GKeyCodeCfg,"GKeyCodes.cfg",1024-strlen(GKeyCodeCfg));
	printf("%s\n",GKeyCodeCfg);
	getKeyDefs(GKeyCodeCfg);


    configure_mmediakeys();
    change_keymap(0);
    XFlush(dpy);

    if(keysonly>0)
      goto close_and_exit;

    /* old binary config format */
    strncat(configpath,"/.g15macro",1024-strlen(configpath));
    strncat(configpath,"/g15macro-data",1024-strlen(configpath));
    config_fd = open(configpath,O_RDONLY|O_SYNC);

    mstates[0] = malloc(sizeof(mstates_t));
    mstates[1] = (mstates_t*)malloc(sizeof(mstates_t));
    mstates[2] = (mstates_t*)malloc(sizeof(mstates_t));

    if(config_fd>0) {
        printf("Converting old data\n");
        read(config_fd,mstates[0],sizeof(mstates_t));
        read(config_fd,mstates[1],sizeof(mstates_t));
        read(config_fd,mstates[2],sizeof(mstates_t));
        close(config_fd);
        strncpy(configpath,getenv("HOME"),1024);
        strncat(configpath,"/.g15macro",1024-strlen(configpath));
        char configbak[1024];
        strcpy(configbak,configpath);
        strncat(configpath,"/g15macro-data",1024-strlen(configpath));
        strncat(configbak,"/g15macro-data.old",1024-strlen(configpath));
        rename(configpath,configbak);
        convert = 1;
    }
	else
		cleanMstates(0); //0 = only NULL the pointers

    /* new format */
    strncpy(configpath,getenv("HOME"),1024);
    strncat(configpath,"/.g15macro",1024-strlen(configpath));
    mkdir(configpath,0777);
    strncat(configpath,"/g15macro.conf",1024-strlen(configpath));
    config=fopen(configpath,"a");
    fclose(config);

    do {
      if((g15screen_fd = new_g15_screen(G15_G15RBUF))<0){
        printf("Sorry, cant connect to the G15daemon - retrying\n");
        sleep(2);
      }
    }while(g15screen_fd<0);

	loadMultiConfig();
	printf("I've now got the following macro files:\n");
	for(i=0; i < MAX_CONFIGS;++i)
	{
		if(!configs[i])
			continue;
		printf("%i:%s%s\n",i,configDir,configs[i]->configfile);
	}

	if(!convert)
	{
		memset(configpath,0,sizeof(configpath));
		strcpy(configpath,configDir);
		strncat(configpath,configs[currConfig]->configfile,1024-strlen(configpath));
		restore_config(configpath);
	}


    if(dump){
        printf("G15Macro Dumping Codes...");
        dump_config(stderr);
        exit(0);
    }

    g15_send_cmd (g15screen_fd,G15DAEMON_KEY_HANDLER, dummy);
    usleep(1000);
    g15_send_cmd (g15screen_fd,G15DAEMON_MKEYLEDS,mled_state);
    usleep(1000);

    root_win = DefaultRootWindow(dpy);
    if (!root_win) {
        printf("Cant find root window\n");
        return 1;
    }


    have_xtest = False;
#ifdef HAVE_XTEST
#ifdef USE_XTEST
    have_xtest = XTestQueryExtension(dpy, &dummy, &dummy, &xtest_major_version, &xtest_minor_version);
    if(have_xtest == False || xtest_major_version < 2 || (xtest_major_version <= 2 && xtest_minor_version < 2))
    {
        printf("Warning: XTEST extension not supported by Xserver.  This is not fatal.\nReverting to XSendEvent for keypress emulation\n");
    }
#else //USE_XTEST
  printf("XTest disabled by configure option.  Using XSendEvent instead.\n");
#endif //USE_XTEST
#else //HAVE_XTEST
  printf("XTest disabled by configure: no devel package was found.  Using XSendEvent instead.\n");
#endif //HAVE_XTEST

	printf("XTest enabled. Using XTest.\n");


	new_action.sa_handler = g15macro_sighandler;
	new_action.sa_flags = 0;
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGQUIT, &new_action, NULL);
	sigaction(SIGTERM, &new_action, NULL);
	sigaction(SIGPIPE, &new_action, NULL);
	sigaction(SIGHUP, &new_action, NULL);

	// So that forked processes that die can actually die instead of going defunct.
	struct sigaction act;
	memset(&act,0,sizeof(act));
	act.sa_handler = &cleanupChildren;
	sigaction(SIGCHLD,&act,NULL);


    snprintf((char*)splashpath,1024,"%s/%s",DATADIR,"g15macro/splash/g15macro.wbmp");
    g15r_loadWbmpSplash(canvas, splashpath);
    g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
	// Following piece of code is not documented (what i could find anyway)
	// But makes so that the user can never bring this screen to front.
	// TODO: Document it
//     #ifdef G15DAEMON_NEVER_SELECT
//         g15_send_cmd (g15screen_fd, G15DAEMON_NEVER_SELECT, dummy);
//     #endif

    usleep(1000);
    pthread_mutex_init(&x11mutex,NULL);
    pthread_mutex_init(&config_mutex,NULL);
	pthread_mutex_init(&gui_select,NULL);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int thread_policy=SCHED_FIFO;
    pthread_attr_setschedpolicy(&attr,thread_policy);
    pthread_attr_setstacksize(&attr,32*1024); /* set stack to 32k - dont need 8Mb !! */

    pthread_create(&Xkeys, &attr, xevent_thread, NULL);
    pthread_create(&Lkeys, &attr, Lkeys_thread, NULL);


	mainLoop();

	cleanup();

close_and_exit:
    /*    XCloseDisplay(dpy);  */
    return 0;
}
