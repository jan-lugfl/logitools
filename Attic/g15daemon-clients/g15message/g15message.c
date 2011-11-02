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

        $Revision: 493 $ -  $Date: 2009-04-24 23:48:21 -0700 (Fri, 24 Apr 2009) $ $Author: mlampard $

        This daemon listens on localhost port 15550 for client connections,
        and arbitrates LCD display.  Allows for multiple simultaneous clients.
        Client screens can be cycled through by pressing the 'L1' key.

        This is a simple Alert/Messege displayer for the G15.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>

#include <g15daemon_client.h>
#include <libg15.h>
#include <libg15render.h>
#include <poll.h>
#include <pthread.h>
#include <math.h>

#ifndef G15R_FONT_API_V2
#error libg15render 1.4 needed
#endif

int g15screen_fd, retval;
g15canvas *canvas;

int handle_Lkeys() {
    int keystate = 0;
    struct pollfd fds;
    char ver[5];
    int foo = 0;

    strncpy(ver,G15DAEMON_VERSION,3);
    float g15v;
    sscanf(ver,"%f",&g15v);

    fds.fd = g15screen_fd;
    fds.events = POLLIN;
    
    if((g15v*10)<=18) {
        keystate = g15_send_cmd (g15screen_fd, G15DAEMON_GET_KEYSTATE, foo);
    } else {
        if ((poll(&fds, 1, 5)) > 0)
            read (g15screen_fd, &keystate, sizeof (keystate));
    }

    if (keystate)
    {
        return keystate;
    }
    return -1;
}

/* Prints text to an area, tries to cut lines between words. */
static void render_text_area(g15canvas *canvas, char* text, int y, int fontsize, int centered) {

    /* The position of the text to be printed. */
    char* cursor = text;

    /* The line to be printed next. */
    char text_line[512];
    int line_width;
    int old_line_length, line_cursor;
    int line_no=0;
    char* bound, *old_bound;
    int x = 0;
    int pixels_per_line = 160;
    g15font *font = g15r_requestG15DefaultFont (fontsize);
    int max_lines =  ceil((43-y) / font->lineheight);
    /* shrink font if necessary to show whole text */

    while ((g15r_testG15FontWidth(font,text) > (pixels_per_line * max_lines) && fontsize)) {
        font = g15r_requestG15DefaultFont (--fontsize);
        max_lines =  ceil((43-y) / font->lineheight);
    }

    while (cursor < text + strlen(text)) {
        bound = cursor;
        line_cursor = 0;
        text_line[0] = '\0';
        /* Find out how much fits in a row. */
        do {
            /* Find out the next word boundary. */
            old_bound = bound;
            old_line_length = strlen(text_line);
            line_cursor = old_line_length;

            /* Strip leading white space. */
            while (isspace(*bound)) 
                bound++;

            /* Copy text to the text_line until end of word or end of string. */
            while (!isspace(*bound) && *bound != '\0') {
                text_line[line_cursor] = *bound;
                bound++;
                line_cursor++;
            }
            text_line[line_cursor++] = ' ';
            text_line[line_cursor] = '\0';

            /* Try if the line with a new word still fits to the given area. */
            line_width = g15r_testG15FontWidth(font,text_line);

            if (line_width > pixels_per_line) {
                /* It didn't fit, restore the old line and stop trying to fit more.*/
                old_line_length--;
                text_line[old_line_length] = '\0';
                
                /* If no words did fit to the line, fit as many characters as possible in it. */
                if (old_line_length == 0) {
                    line_width = 0;
                    bound = bound - line_cursor + 1; /* rewind to the beginning of the word */
                    line_cursor = 0;
                    while (!isspace(*bound) && *bound != '\0') {
                        text_line[line_cursor++] = *bound++;
                        text_line[line_cursor] = '\0';
                        /* The last character did not fit. */
                        if (x + line_width >= pixels_per_line) {
                            text_line[line_cursor - 1] = '\0';
                            bound--;
                            break;
                        }
                    }
                    /* The line is now filled with chars from the word. */
                    break;
                }
                bound = old_bound;
                break;
            }
  
            /* OK, it did fit, let's try to fit some more. */
        } while (bound < text + strlen(text));
        /* only render up to max_lines */
        if (line_no>=max_lines) {
            break;
        }
        
        g15r_G15FPrint (canvas, text_line, 0, y, fontsize, centered, G15_COLOR_BLACK, line_no);
        line_no++;

        cursor = bound;
    }

}
int main(int argc, char **argv)
{
    int i=0;
    char * message = NULL;
    char * title = NULL;
    int show_title = 0;
    int wait_for_confirmation = 0;
    unsigned int delay = 5;
    unsigned int force_foreground = 0;
    unsigned int y_offset=0;
    int lines_available=6;
    int leaving = 0;
    int centered = 0;
    int fontsize = 0;
    g15font *font = NULL;
        
    if(argc<2) {
        printf("%s - A simple message displayer for the G15\n",argv[0]);
        printf("Usage: %s <args> \"message\"\n",argv[0]);
        printf(" -t \"Title\" Alert title\n");
        printf(" -y Wait for Yes/No Confirmation before exiting (returns 0/1 on exit)\n");
        printf(" -o Wait for Ok Confirmation before exiting\n");
        printf(" -c Centre align message text\n");
        printf(" -d <secs> show alert for <secs> seconds then exit (default) - overrides the interactive wait options, above.\n");
        printf(" -f force stay in foreground until timeout/confirmation.\n");
        printf(" -s <size> render message with <size> sized text.\n");

        printf("\nMessage text will be wrapped to fit on the display\nWith no timeout or wait options message is displayed for 5seconds\n\n");
        exit(0);
    }

    for(i=1;i<argc;i++){
        if(argv[i][0]!='-'){
            message = malloc(strlen(argv[i])+1);
            memset(message,0,strlen(argv[i])+1);
            strcpy(message,argv[i]);
        }

        if(0==strncasecmp(argv[i],"-t",2)){
            if(argv[i+1]) {
                if(argv[i+1][0]!='-') {
                    i++;
                    title = malloc(strlen(argv[i]));
                    strcpy(title,argv[i]);
                    show_title=1;
                    lines_available-=2;
                }
            }
        }

        if(0==strncasecmp(argv[i],"-y",2)){
            wait_for_confirmation = 1;
            delay = 0;
            lines_available--;
        }
        
        if(0==strncasecmp(argv[i],"-c",2)){
            centered = 1;
        }

        if(0==strncasecmp(argv[i],"-o",2)){
            wait_for_confirmation = 2;
            delay = 0;
            lines_available--;
        }

        if(0==strncasecmp(argv[i],"-f",2)){
            force_foreground = 1;
        }

        if(0==strncasecmp(argv[i],"-d",2)){
            if(argv[i+1]!=NULL){
                if(isdigit(argv[i+1][0])){
                    wait_for_confirmation = 0;
                    i++;
                    delay = strtol(argv[i],NULL,10);
                }
            }
        }
        if(0==strncasecmp(argv[i],"-s",2)){
            if(argv[i+1]!=NULL){
                if(isdigit(argv[i+1][0])){
                    i++;
                    fontsize = strtol(argv[i],NULL,10);
                    font = g15r_requestG15DefaultFont (fontsize);
                }
            }
        }
    }
    if(!fontsize) {
        fontsize = 10;
        font = g15r_requestG15DefaultFont (fontsize);
    }
    if(message==NULL||(g15screen_fd = new_g15_screen(G15_G15RBUF))<0){
        if(message==NULL)
            printf("No message - nothing to do. exiting\n");
        else
            printf("Couldnt connect to the G15daemon. Exiting\n");
        
        if(canvas!=NULL)
            free(canvas);

        if(message!=NULL)
            free(message);

        if(title!=NULL)
            free(title);

        return 255;
    }

    canvas = (g15canvas *) malloc (sizeof (g15canvas));
    if (canvas != NULL) {
        memset(canvas->buffer, 0, G15_BUFFER_LEN);
        canvas->mode_cache = 0;
        canvas->mode_reverse = 0;
        canvas->mode_xor = 0;
    }

    if(show_title){
        canvas->mode_xor=1;
        g15r_pixelBox (canvas, 0, 0, 159 , 9, G15_COLOR_BLACK, 1, 1);
        g15r_G15FPrint (canvas,title,0,1,8,G15_JUSTIFY_CENTER,G15_COLOR_BLACK,0);
        canvas->mode_xor=0;
        y_offset = 12;
    }

    if(g15r_testG15FontWidth(font,message)>160)
        render_text_area(canvas, message, y_offset, fontsize, centered);
    else
        g15r_G15FPrint (canvas, message, 0, (21-(wait_for_confirmation==1?5:0)+(show_title==1?7:0))-(font->ascender_height/2), fontsize, G15_JUSTIFY_CENTER, G15_COLOR_BLACK, 0);
    

    if(wait_for_confirmation) {
        g15r_pixelBox (canvas, 0, 34, 159 , 42, G15_COLOR_WHITE, 1, 1);
        switch(wait_for_confirmation) {
            case 1: {
                canvas->mode_xor=1;
                g15r_pixelBox (canvas, 104, 34, 125 , 42, G15_COLOR_BLACK, 1, 1);
                g15r_renderString (canvas, (unsigned char *)"Yes", 0, G15_TEXT_MED, 108, 36);
                g15r_pixelBox (canvas, 130, 34, 149 , 42, G15_COLOR_BLACK, 1, 1);
                g15r_renderString (canvas, (unsigned char *)"No", 0, G15_TEXT_MED, 136, 36);
                canvas->mode_xor=0;
                break;
            }
            case 2: {
                canvas->mode_xor=1;
                g15r_pixelBox (canvas, 130, 34, 149 , 42, G15_COLOR_BLACK, 1, 1);
                g15r_renderString (canvas, (unsigned char *)"Ok", 0, G15_TEXT_MED, 136, 36);
                canvas->mode_xor=0;
                break;
            }
        }
    }
    g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN);
    retval = 0;
    if(wait_for_confirmation){
        while(!leaving){
            if(force_foreground){ // remain in foreground 
                int foo = 0;
                int fg_check = g15_send_cmd (g15screen_fd, G15DAEMON_IS_FOREGROUND, foo);
                if(fg_check==0) // not foreground ??
                    g15_send_cmd (g15screen_fd, G15DAEMON_SWITCH_PRIORITIES, foo); // switch priorities with the foreground client
            }

            int keypress = handle_Lkeys();
            switch(keypress)
            {
                case G15_KEY_L4: {
                    if(wait_for_confirmation == 1) {
                        leaving =1;
                        retval = 1;
                    }
                    break;
                    case G15_KEY_L5: {
                        leaving =1;
                        retval = 0;
                    }
                    break;
                    default:
                        break;
                }
            }
        }
    }
    sleep(delay);

    if(canvas!=NULL)
        free(canvas);

    if(message!=NULL)
        free(message);

    if(title!=NULL)
        free(title);

    close(g15screen_fd);

    return retval;
}
