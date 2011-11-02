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
          
     $Revision: 348 $ -  $Date: 2007-12-19 21:10:36 -0800 (Wed, 19 Dec 2007) $ $Author: mlampard $
              
    This daemon listens on localhost port 15550 for client connections,
    and arbitrates LCD display.  Allows for multiple simultaneous clients.
    Client screens can be cycled through by pressing the 'L1' key.
*/

#define G15DAEMON

#ifndef BLACKnWHITE
#define BLACK 1
#define WHITE 0
#define BLACKnWHITE 
#endif

#define GKEY_OFFSET 167
#define MKEY_OFFSET 185
#define LKEY_OFFSET 189

#define G15KEY_DOWN 1
#define G15KEY_UP 0

#define LCD_WIDTH 160
#define LCD_HEIGHT 43

/* tcp server defines */
#define LISTEN_PORT 15550
#define LISTEN_ADDR "127.0.0.1"
/* any more than this number of simultaneous clients will be rejected. */
#define MAX_CLIENTS 10

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <pthread.h>

#define CLIENT_CMD_GET_KEYSTATE 'k'
#define CLIENT_CMD_SWITCH_PRIORITIES 'p'
#define CLIENT_CMD_IS_FOREGROUND 'v'
#define CLIENT_CMD_IS_USER_SELECTED 'u'
#define CLIENT_CMD_BACKLIGHT 0x80
#define CLIENT_CMD_CONTRAST 0x40
#define CLIENT_CMD_MKEY_LIGHTS 0x20
/* if the following CMD is sent from a client, G15Daemon will not send any MR or G? keypresses via uinput, 
 * all M&G keys must be handled by the client.  If the client dies or exits, normal functions resume. */
#define CLIENT_CMD_KEY_HANDLER 0x10

typedef struct lcd_s
{
    int lcd_type;
    unsigned char buf[1048];
    int max_x;
    int max_y;
    int connection;
    long int ident;
    unsigned int backlight_state;
    unsigned int mkey_state;
    unsigned int contrast_state;
    unsigned int state_changed;
    /* set to 1 if user manually selected this screen 0 otherwise*/
    unsigned int usr_foreground;
} lcd_t;

typedef struct lcdnode_s lcdnode_t;
typedef struct lcdlist_s lcdlist_t;

struct lcdnode_s {
    lcdlist_t *list;
    lcdnode_t *prev;
    lcdnode_t *next;
    lcdnode_t *last_priority;
    lcd_t *lcd;
}lcdnode_s;

struct lcdlist_s
{
    lcdnode_t *head;
    lcdnode_t *tail;
    lcdnode_t *current;
}lcdlist_s;

pthread_mutex_t lcdlist_mutex;
pthread_mutex_t g15lib_mutex;

/* server hello */
#define SERV_HELO "G15 daemon HELLO"

/* uinput & keyboard control */
#ifdef HAVE_LINUX_UINPUT_H
int g15_init_uinput();
void g15_uinput_keyup(unsigned char code);
void g15_uinput_keydown(unsigned char code);
void g15_exit_uinput();
#endif
    
void g15_process_keys(lcdlist_t *displaylist, unsigned int currentkeys, unsigned int lastkeys);

/* call create_lcd for every new client, and quit it when done */
lcd_t * create_lcd ();
void quit_lcd (lcd_t * lcd);
void write_buf_to_g15(lcd_t *lcd);

void setpixel (lcd_t * lcd, unsigned int x1, unsigned int y1, unsigned int color);
void cls (lcd_t * lcd, int color);
void line (lcd_t * lcd, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned int color);
void rectangle (lcd_t * lcd, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, int filled, unsigned int color);
void circle (lcd_t * lcd, int x, int y, int r, int filled, int color);
void roundrectangle (lcd_t * lcd, int x1, int y1, int x2, int y2, int filled, int color, int type);
void draw_bitmap_char (lcd_t * lcd, unsigned char chr, int x, int y, int color, int doublesize, int bold);
void draw_bitmap_str (lcd_t * lcd, char *str, int x, int y, int color, int size, int bold);
void lcd_printf(lcd_t *lcd, int x, int y, const char *fmt, ...);
void draw_bar (lcd_t * lcd, int x1, int y1, int x2, int y2, int color, int num, int max);
void draw_bignum (lcd_t * lcd, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned int color, int num);
void lcdclock(lcd_t *lcd);
/* utility functions in utility_func.c */
void pthread_sleep(int seconds);
int pthread_msleep(int milliseconds);

/* linked lists */
lcdlist_t *lcdlist_init();
void lcdlist_destroy(lcdlist_t **displaylist);
lcdnode_t *lcdnode_add(lcdlist_t **display_list);
void lcdnode_remove(lcdnode_t *badnode);

/* create a listening socket */
int init_sockserver();
int g15_clientconnect(lcdlist_t **g15daemon,int listening_socket);
int g15_send(int sock, char *buf, unsigned int len);
int g15_recv(lcdnode_t *lcdnode, int sock, char *buf, unsigned int len);

/* handy function from xine_utils.c */
void *g15_xmalloc(size_t size);

void send_keystate(lcd_t *client);
