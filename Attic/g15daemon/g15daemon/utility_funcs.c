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

    $Revision: 525 $ -  $Date: 2010-01-24 05:38:22 -0800 (Sun, 24 Jan 2010) $ $Author: steelside $
        
    This daemon listens on localhost port 15550 for client connections,
    and arbitrates LCD display.  Allows for multiple simultaneous clients.
    Client screens can be cycled through by pressing the 'L1' key.
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
#include <poll.h>
#include <sys/socket.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libdaemon/daemon.h>
#include "g15daemon.h"
#include <libg15.h>

extern int leaving;
extern unsigned int cycle_key;
extern unsigned int client_handles_keys;

unsigned int connected_clients = 0;

#ifdef HAVE_LINUX_UINPUT_H
    void (*keyup)(unsigned char code) = &g15_uinput_keyup;
    void (*keydown)(unsigned char code) = &g15_uinput_keydown;
#else
    void keyup(unsigned char code) { printf("Extra Keys not supported due to missing Uinput.h\n"); }
    void keydown(unsigned char code) { printf("Extra Keys not supported due to missing Uinput.h\n"); }
#endif

void g15_process_keys(lcdlist_t *displaylist, unsigned int currentkeys, unsigned int lastkeys){
    

    /* 'G' keys */
    if(!client_handles_keys) {
        if((currentkeys & G15_KEY_G1) && !(lastkeys & G15_KEY_G1))
            keydown(GKEY_OFFSET);
        else if(!(currentkeys & G15_KEY_G1) && (lastkeys & G15_KEY_G1))
            keyup(GKEY_OFFSET);

        if((currentkeys & G15_KEY_G2) && !(lastkeys & G15_KEY_G2))
            keydown(GKEY_OFFSET+1);
        else if(!(currentkeys & G15_KEY_G2) && (lastkeys & G15_KEY_G2))
            keyup(GKEY_OFFSET+1);

        if((currentkeys & G15_KEY_G3) && !(lastkeys & G15_KEY_G3))
            keydown(GKEY_OFFSET+2);
        else if(!(currentkeys & G15_KEY_G3) && (lastkeys & G15_KEY_G3))
            keyup(GKEY_OFFSET+2);

        if((currentkeys & G15_KEY_G4) && !(lastkeys & G15_KEY_G4))
            keydown(GKEY_OFFSET+3);
        else if(!(currentkeys & G15_KEY_G4) && (lastkeys & G15_KEY_G4))
            keyup(GKEY_OFFSET+3);

        if((currentkeys & G15_KEY_G5) && !(lastkeys & G15_KEY_G5))
            keydown(GKEY_OFFSET+4);
        else if(!(currentkeys & G15_KEY_G5) && (lastkeys & G15_KEY_G5))
            keyup(GKEY_OFFSET+4);

        if((currentkeys & G15_KEY_G6) && !(lastkeys & G15_KEY_G6))
            keydown(GKEY_OFFSET+5);
        else if(!(currentkeys & G15_KEY_G6) && (lastkeys & G15_KEY_G6))
            keyup(GKEY_OFFSET+5);

        if((currentkeys & G15_KEY_G7) && !(lastkeys & G15_KEY_G7))
            keydown(GKEY_OFFSET+6);
        else if(!(currentkeys & G15_KEY_G7) && (lastkeys & G15_KEY_G7))
            keyup(GKEY_OFFSET+6);

        if((currentkeys & G15_KEY_G8) && !(lastkeys & G15_KEY_G8))
            keydown(GKEY_OFFSET+7);
        else if(!(currentkeys & G15_KEY_G8) && (lastkeys & G15_KEY_G8))
            keyup(GKEY_OFFSET+7);

        if((currentkeys & G15_KEY_G9) && !(lastkeys & G15_KEY_G9))
            keydown(GKEY_OFFSET+8);
        else if(!(currentkeys & G15_KEY_G9) && (lastkeys & G15_KEY_G9))
            keyup(GKEY_OFFSET+8);

        if((currentkeys & G15_KEY_G10) && !(lastkeys & G15_KEY_G10))
            keydown(GKEY_OFFSET+9);
        else if(!(currentkeys & G15_KEY_G10) && (lastkeys & G15_KEY_G10))
            keyup(GKEY_OFFSET+9);

        if((currentkeys & G15_KEY_G11) && !(lastkeys & G15_KEY_G11))
            keydown(GKEY_OFFSET+10);
        else if(!(currentkeys & G15_KEY_G11) && (lastkeys & G15_KEY_G11))
            keyup(GKEY_OFFSET+10);

        if((currentkeys & G15_KEY_G12) && !(lastkeys & G15_KEY_G12))
            keydown(GKEY_OFFSET+11);
        else if(!(currentkeys & G15_KEY_G12) && (lastkeys & G15_KEY_G12))
            keyup(GKEY_OFFSET+11);

        if((currentkeys & G15_KEY_G13) && !(lastkeys & G15_KEY_G13))
            keydown(GKEY_OFFSET+12);
        else if(!(currentkeys & G15_KEY_G13) && (lastkeys & G15_KEY_G13))
            keyup(GKEY_OFFSET+12);

        if((currentkeys & G15_KEY_G14) && !(lastkeys & G15_KEY_G14))
            keydown(GKEY_OFFSET+13);
        else if(!(currentkeys & G15_KEY_G14) && (lastkeys & G15_KEY_G14))
            keyup(GKEY_OFFSET+13);

        if((currentkeys & G15_KEY_G15) && !(lastkeys & G15_KEY_G15))
            keydown(GKEY_OFFSET+14);
        else if(!(currentkeys & G15_KEY_G15) && (lastkeys & G15_KEY_G15))
            keyup(GKEY_OFFSET+14);

        if((currentkeys & G15_KEY_G16) && !(lastkeys & G15_KEY_G16))
            keydown(GKEY_OFFSET+15);
        else if(!(currentkeys & G15_KEY_G16) && (lastkeys & G15_KEY_G16))
            keyup(GKEY_OFFSET+15);

        if((currentkeys & G15_KEY_G17) && !(lastkeys & G15_KEY_G17))
            keydown(GKEY_OFFSET+16);
        else if(!(currentkeys & G15_KEY_G17) && (lastkeys & G15_KEY_G17))
            keyup(GKEY_OFFSET+16);

        if((currentkeys & G15_KEY_G18) && !(lastkeys & G15_KEY_G18))
            keydown(GKEY_OFFSET+17);
        else if(!(currentkeys & G15_KEY_G18) && (lastkeys & G15_KEY_G18))
            keyup(GKEY_OFFSET+17);

        /* 'M' keys */

        if((currentkeys & G15_KEY_M1) && !(lastkeys & G15_KEY_M1))
            keydown(MKEY_OFFSET);
        else if(!(currentkeys & G15_KEY_M1) && (lastkeys & G15_KEY_M1))
            keyup(MKEY_OFFSET);

        if((currentkeys & G15_KEY_M2) && !(lastkeys & G15_KEY_M2))
            keydown(MKEY_OFFSET+1);
        else if(!(currentkeys & G15_KEY_M2) && (lastkeys & G15_KEY_M2))
            keyup(MKEY_OFFSET+1);

        if((currentkeys & G15_KEY_M3) && !(lastkeys & G15_KEY_M3))
            keydown(MKEY_OFFSET+2);
        else if(!(currentkeys & G15_KEY_M3) && (lastkeys & G15_KEY_M3))
            keyup(MKEY_OFFSET+2);
    }
    if(!connected_clients) {
      if(cycle_key != G15_KEY_MR) {
          if(!client_handles_keys) {
              if((currentkeys & G15_KEY_MR) && !(lastkeys & G15_KEY_MR))
                  keydown(MKEY_OFFSET+3);
              else if(!(currentkeys & G15_KEY_MR) && (lastkeys & G15_KEY_MR))
                  keyup(MKEY_OFFSET+3);
          }
      }
    }else{
        /* cycle through connected client displays if L1 is pressed */
        if((currentkeys & cycle_key) && !(lastkeys & cycle_key))
        {
            pthread_mutex_lock(&lcdlist_mutex);
            lcdnode_t *current_screen = displaylist->current;
        	do
        	{
        		displaylist->current->lcd->usr_foreground = 0;
        		if(displaylist->tail == displaylist->current)
        			displaylist->current = displaylist->head;
        		else
        			displaylist->current = displaylist->current->prev;
        	} 
        	while (current_screen != displaylist->current);
            if(displaylist->tail == displaylist->current) {
                displaylist->current = displaylist->head;
            } else {
                displaylist->current = displaylist->current->prev;
            }
            displaylist->current->lcd->usr_foreground = 1;
            displaylist->current->lcd->state_changed = 1;
            displaylist->current->last_priority =  displaylist->current;
            pthread_mutex_unlock(&lcdlist_mutex);
        }
    }
    
    /* 'L' keys...  */
    if(cycle_key!=G15_KEY_L1) {    
      if((currentkeys & G15_KEY_L1) && !(lastkeys & G15_KEY_L1))
        keydown(LKEY_OFFSET);
      else if(!(currentkeys & G15_KEY_L1) && (lastkeys & G15_KEY_L1))
        keyup(LKEY_OFFSET);
    }
    
    if((currentkeys & G15_KEY_L2) && !(lastkeys & G15_KEY_L2))
        keydown(LKEY_OFFSET+1);
    else if(!(currentkeys & G15_KEY_L2) && (lastkeys & G15_KEY_L2))
        keyup(LKEY_OFFSET+1);

    if((currentkeys & G15_KEY_L3) && !(lastkeys & G15_KEY_L3))
        keydown(LKEY_OFFSET+2);
    else if(!(currentkeys & G15_KEY_L3) && (lastkeys & G15_KEY_L3))
        keyup(LKEY_OFFSET+2);

    if((currentkeys & G15_KEY_L4) && !(lastkeys & G15_KEY_L4))
        keydown(LKEY_OFFSET+3);
    else if(!(currentkeys & G15_KEY_L4) && (lastkeys & G15_KEY_L4))
        keyup(LKEY_OFFSET+3);

    if((currentkeys & G15_KEY_L5) && !(lastkeys & G15_KEY_L5))
        keydown(LKEY_OFFSET+4);
    else if(!(currentkeys & G15_KEY_L5) && (lastkeys & G15_KEY_L5))
        keyup(LKEY_OFFSET+4);

}

/* handy function from xine_utils.c */
void *g15_xmalloc(size_t size) {
    void *ptr;

    /* prevent xmalloc(0) of possibly returning NULL */
    if( !size )
        size++;

    if((ptr = calloc(1, size)) == NULL) {
        daemon_log(LOG_WARNING, "g15_xmalloc() failed: %s.\n", strerror(errno));
        return NULL;
    }
    return ptr;
}

void convert_buf(lcd_t *lcd, unsigned char * orig_buf)
{
    unsigned int x,y;
    for(x=0;x<160;x++)
        for(y=0;y<43;y++)
            setpixel(lcd,x,y,orig_buf[x+(y*160)]);
}
                                        

/* wrap the libg15 function */
void write_buf_to_g15(lcd_t *lcd)
{
    pthread_mutex_lock(&g15lib_mutex);
    writePixmapToLCD(lcd->buf);
    pthread_mutex_unlock(&g15lib_mutex);
    return;
}


/* Sleep routine (hackish). */
void pthread_sleep(int seconds) {
    pthread_mutex_t dummy_mutex;
    static pthread_cond_t dummy_cond = PTHREAD_COND_INITIALIZER;
    struct timespec timeout;

    /* Create a dummy mutex which doesn't unlock for sure while waiting. */
    pthread_mutex_init(&dummy_mutex, NULL);
    pthread_mutex_lock(&dummy_mutex);

    timeout.tv_sec = time(NULL) + seconds;
    timeout.tv_nsec = 0;

    pthread_cond_timedwait(&dummy_cond, &dummy_mutex, &timeout);

    /*    pthread_cond_destroy(&dummy_cond); */
    pthread_mutex_unlock(&dummy_mutex);
    pthread_mutex_destroy(&dummy_mutex);
}

/* millisecond sleep routine. */
int pthread_msleep(int milliseconds) {
    
    struct timespec timeout;
    if(milliseconds>999)
        milliseconds=999;
    timeout.tv_sec = 0;
    timeout.tv_nsec = milliseconds*1000000;

    return nanosleep (&timeout, NULL);
}


void lcdclock(lcd_t *lcd)
{
    unsigned int col = 0;
    unsigned int len=0;
    int narrows=0;
    int totalwidth=0;
    char buf[10];
    
    time_t currtime = time(NULL);
    
    if(lcd->ident < currtime - 60) {	
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
        lcd->ident = currtime;
    }
}


/* the client must send 6880 bytes for each lcd screen.  This thread will continue to copy data
* into the clients LCD buffer for as long as the connection remains open. 
* so, the client should open a socket, check to ensure that the server is a g15daemon,
* and send multiple 6880 byte packets (1 for each screen update) 
* once the client disconnects by closing the socket, the LCD buffer is 
* removed and will no longer be displayed.
*/
void *lcd_client_thread(void *display) {

    lcdnode_t *g15node = display;
    lcd_t *client_lcd = g15node->lcd;
    int retval;
    int i,y,x;
    unsigned int width, height, buflen,header=4;

    int client_sock = client_lcd->connection;
    char helo[]=SERV_HELO;
    unsigned char *tmpbuf=g15_xmalloc(6880);
    
    if(!connected_clients)
        setLEDs(G15_LED_MR); /* turn on the MR backlight to show that it's now being used for lcd-switching */
    connected_clients++;
    
    if(g15_send(client_sock, (char*)helo, strlen(SERV_HELO))<0){
        goto exitthread;
    }
    /* check for requested buffer type.. we only handle pixel buffers atm */
    if(g15_recv(g15node, client_sock,(char*)tmpbuf,4)<4)
        goto exitthread;

    /* we will in the future handle txt buffers gracefully but for now we just hangup */
    if(tmpbuf[0]=='G') {
        while(!leaving) {
            retval = g15_recv(g15node, client_sock,(char *)tmpbuf,6880);
            if(retval!=6880){
                break;
            }
            pthread_mutex_lock(&lcdlist_mutex);
            memset(client_lcd->buf,0,1024);      
            convert_buf(client_lcd,tmpbuf);
            client_lcd->ident = random();
            pthread_mutex_unlock(&lcdlist_mutex);
        }
    }
    else if (tmpbuf[0]=='R') { /* libg15render buffer */
        while(!leaving) {
            retval = g15_recv(g15node, client_sock, (char *)tmpbuf, 1048);
            if(retval != 1048) {
                break;
            }
            pthread_mutex_lock(&lcdlist_mutex);
            memcpy(client_lcd->buf,tmpbuf,sizeof(client_lcd->buf));
            client_lcd->ident = random();
            pthread_mutex_unlock(&lcdlist_mutex);
        }
    }
    else if (tmpbuf[0]=='W'){ /* wbmp buffer - we assume (stupidly) that it's 160 pixels wide */
        while(!leaving) {
            retval = g15_recv(g15node, client_sock,(char*)tmpbuf, 865);
            if(!retval)
                break;

            if (tmpbuf[2] & 1) {
                width = ((unsigned char)tmpbuf[2] ^ 1) | (unsigned char)tmpbuf[3];
                height = tmpbuf[4];
                header = 5;
            } else {
                width = tmpbuf[2];
                height = tmpbuf[3];
                header = 4;
            }
            
            buflen = (width/8)*height;

            if(buflen>860){ /* grab the remainder of the image and discard excess bytes */
                /*  retval=g15_recv(client_sock,(char*)tmpbuf+865,buflen-860);  */
                retval=g15_recv(g15node, client_sock,NULL,buflen-860); 
                buflen = 860;
            }
            
            if(width!=160) /* FIXME - we ought to scale images I suppose */
                goto exitthread;
            
            pthread_mutex_lock(&lcdlist_mutex);
            memcpy(client_lcd->buf,tmpbuf+header,buflen+header);
            client_lcd->ident = random();
            pthread_mutex_unlock(&lcdlist_mutex);
        }
    }
exitthread:
        close(client_sock);
    free(tmpbuf);
    lcdnode_remove(display);
    connected_clients--;
    if(!connected_clients)
        setLEDs(0);
    pthread_exit(NULL);
}

/* poll the listening socket for connections, spawning new threads as needed to handle clients */
int g15_clientconnect (lcdlist_t **g15daemon, int listening_socket) {

    int conn_s;
    struct pollfd pfd[1];
    pthread_t client_connection;
    pthread_attr_t attr;
    lcdnode_t *clientnode;

    memset(pfd,0,sizeof(pfd));
    pfd[0].fd = listening_socket;
    pfd[0].events = POLLIN;


    if (poll(pfd,1,500)>0){
        if (!(pfd[0].revents & POLLIN)){
            return 0;
        }

        if ( (conn_s = accept(listening_socket, NULL, NULL) ) < 0 ) {
            if(errno==EWOULDBLOCK || errno==EAGAIN){
            }else{
                daemon_log(LOG_WARNING, "error calling accept()\n");
                return -1;
            }
        }

        clientnode = lcdnode_add(g15daemon);
        clientnode->lcd->connection = conn_s;

        memset(&attr,0,sizeof(pthread_attr_t));
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr,256*1024); /* set stack to 768k - dont need 8Mb - this is probably rather excessive also */
        if (pthread_create(&client_connection, &attr, lcd_client_thread, clientnode) != 0) {
            daemon_log(LOG_WARNING,"Unable to create client thread.");
            if (close(conn_s) < 0 ) {
                daemon_log(LOG_WARNING, "error calling close()\n");
                return -1;
            }

        }
        
        pthread_detach(client_connection);
    }
    return 0;
}















