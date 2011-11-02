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
  
  (c) 2006-2007 Mike Lampard, Philip Lawatsch, Antonio Bartolini, and others
  
  This daemon listens on localhost port 15550 for client connections,
  and arbitrates LCD display.  Allows for multiple simultaneous clients.
  Client screens can be cycled through by pressing the 'L1' key.
  
  simple analyser xmms plugin for g15daemon
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#include <audacious/plugin.h>

/* check for old plugin */
#ifndef __AUDACIOUS_PLUGIN_API__
#define OLD_PLUGIN
#endif

#include <audacious/util.h>

#ifdef OLD_PLUGIN
#include <audacious/beepctrl.h>
#else
#include <audacious/auddrct.h>
#endif

#include <audacious/configdb.h>
#include <audacious/playlist.h>

#ifdef OLD_PLUGIN
#include <audacious/titlestring.h>
#endif

#include <libg15.h>
#include <libg15render.h>
#include <g15daemon_client.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include "g15daemon_audacious_spectrum.h"


#ifndef OLD_PLUGIN
VisPlugin *g15_vp[] = { &g15analyser_vp, NULL };
SIMPLE_VISUAL_PLUGIN(g15, g15_vp);
#endif

gint get_main_volume(void) {
  int tmpvol;
  
#ifdef OLD_PLUGIN
  tmpvol = xmms_remote_get_main_volume(g15analyser_vp.xmms_session);
#else
  audacious_drct_get_main_volume(&tmpvol);
#endif
  
  return tmpvol;
}

gint g15analyser_disable(gpointer data){
  g15analyser_vp.disable_plugin (&g15analyser_vp); /* disable if unusable */
  return FALSE;
}

VisPlugin *get_vplugin_info(void) {
  return &g15analyser_vp;
}


void g15spectrum_read_config(void)
{
  ConfigDb *cfg;
#if  __AUDACIOUS_PLUGIN_API__ < 7
  cfg = bmp_cfg_db_open();
#else
  cfg = aud_cfg_db_open();
#endif
  pthread_mutex_lock (&g15buf_mutex);
  if (cfg)
    {
      #if  __AUDACIOUS_PLUGIN_API__ < 7
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "visualisation_type", (int*)&vis_type);
	bmp_cfg_db_get_float(cfg, "G15Daemon Spectrum", "linearity", (float*)&linearity);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "amplification", (int*)&amplification);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "bars_limit", (int*)&limit);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "num_bars", (int*)&num_bars);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "enable_peak", (int*)&enable_peak);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "detached_peak", (int*)&detached_peak);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "line_mode", (int*)&line_mode);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "analog_mode", (int*)&analog_mode);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "analog_step", (int*)&analog_step);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "enable_keybindings", (int*)&enable_keybindings);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "show_title", (int*)&show_title);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "show_pbar", (int*)&show_pbar);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "show_time", (int*)&show_time);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "rownum", (int*)&rownum);
	bmp_cfg_db_get_int(cfg, "G15Daemon Spectrum", "title_overlay", (int*)&title_overlay);
      	bmp_cfg_db_close(cfg);
      #else
	/* New version of audacious */
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "visualisation_type", (int*)&vis_type);
	aud_cfg_db_get_float(cfg, "G15Daemon Spectrum", "linearity", (float*)&linearity);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "amplification", (int*)&amplification);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "bars_limit", (int*)&limit);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "num_bars", (int*)&num_bars);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "enable_peak", (int*)&enable_peak);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "detached_peak", (int*)&detached_peak);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "line_mode", (int*)&line_mode);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "analog_mode", (int*)&analog_mode);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "analog_step", (int*)&analog_step);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "enable_keybindings", (int*)&enable_keybindings);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "show_title", (int*)&show_title);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "show_pbar", (int*)&show_pbar);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "show_time", (int*)&show_time);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "rownum", (int*)&rownum);
	aud_cfg_db_get_int(cfg, "G15Daemon Spectrum", "title_overlay", (int*)&title_overlay);
      #endif
      
    }
  /* check conf integrity */
  if (linearity < 0.1 || linearity > 0.5)
    linearity = def_linearity;
  if (amplification < -30 || amplification > 30)
    amplification = def_amplification;
  if (num_bars > 128 || num_bars <= 1)
    num_bars = def_num_bars;
  if (analog_step < 2 || analog_step > 9)
    analog_step = def_analog_step;
  if (rownum > 4 || rownum < 1)
    rownum = def_rownum;
  if (enable_keybindings != 0 && enable_keybindings != 1)
    enable_keybindings = def_enable_keybindings;
  if (vis_type != FALSE && vis_type != TRUE)
    vis_type = def_vis_type;
  if (enable_peak != FALSE && enable_peak != TRUE)
    enable_peak = def_enable_peak;
  if (detached_peak != FALSE && detached_peak != TRUE)
    detached_peak = def_detached_peak;
  if (analog_mode != FALSE && analog_mode != TRUE)
    analog_mode = def_analog_mode;
  if (line_mode != FALSE && line_mode != TRUE)
    line_mode = def_line_mode;
  if (show_title != FALSE && show_title != TRUE)
    show_title = def_show_title;
  if (show_pbar != FALSE && show_pbar != TRUE)
    show_pbar = def_show_pbar;
  if (show_time != FALSE && show_time != TRUE)
    show_time = def_show_time;
  if (title_overlay != FALSE && title_overlay != TRUE)
    title_overlay = def_title_overlay;
  
  pthread_mutex_unlock (&g15buf_mutex);
  
}

void g15spectrum_write_config(void)
{
  ConfigDb *cfg;
#if  __AUDACIOUS_PLUGIN_API__ < 7
  cfg = bmp_cfg_db_open();
#else
  cfg = aud_cfg_db_open();
#endif 
   
  if (cfg)
    {
#if  __AUDACIOUS_PLUGIN_API__ < 7
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "visualisation_type", vis_type);
      bmp_cfg_db_set_float(cfg, "G15Daemon Spectrum", "linearity", linearity);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "amplification", amplification);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "bars_limit", limit);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "num_bars", num_bars);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "enable_peak", enable_peak);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "detached_peak", detached_peak);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "analog_mode", analog_mode);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "line_mode", line_mode);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "analog_step", analog_step);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "enable_keybindings", enable_keybindings);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "show_title", show_title);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "show_pbar", show_pbar);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "show_time", show_time);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "rownum", rownum);
      bmp_cfg_db_set_int(cfg, "G15Daemon Spectrum", "title_overlay", title_overlay);
      bmp_cfg_db_close(cfg);
#else
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "visualisation_type", vis_type);
      aud_cfg_db_set_float(cfg, "G15Daemon Spectrum", "linearity", linearity);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "amplification", amplification);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "bars_limit", limit);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "num_bars", num_bars);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "enable_peak", enable_peak);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "detached_peak", detached_peak);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "analog_mode", analog_mode);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "line_mode", line_mode);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "analog_step", analog_step);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "enable_keybindings", enable_keybindings);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "show_title", show_title);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "show_pbar", show_pbar);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "show_time", show_time);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "rownum", rownum);
      aud_cfg_db_set_int(cfg, "G15Daemon Spectrum", "title_overlay", title_overlay);
      aud_cfg_db_close(cfg);
#endif

    }
  
}

void g15analyser_conf_apply(void){
  /* Apply gui values */
  pthread_mutex_lock (&g15buf_mutex);
  if (GTK_TOGGLE_BUTTON(t_options_bars_radio)->active)
    vis_type = 0;
  else
    vis_type = 1;
  if ( tmp_lin != -1 )
    linearity = tmp_lin;
  if ( tmp_ampli != -1000 )
    amplification = tmp_ampli;
  if ( tmp_bars != -1 )
    num_bars = (int)tmp_bars;
  if (tmp_step != -1 )
    analog_step = tmp_step;
  if (tmp_rownum != -1 )
    rownum = tmp_rownum;
  if (GTK_TOGGLE_BUTTON(t_options_effect_no)->active){
    enable_peak = 0;
    analog_mode = 0;
    line_mode = 0;
  } else {
    enable_peak = GTK_TOGGLE_BUTTON(t_options_effect_peak)->active;
    analog_mode = GTK_TOGGLE_BUTTON(t_options_effect_analog)->active;
    line_mode = GTK_TOGGLE_BUTTON(t_options_effect_line)->active;
  }
  detached_peak = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_options_enable_dpeak));
  enable_keybindings =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_options_enable_keybindings));
  show_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_options_show_title));
  show_pbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_options_show_pbar));
  show_time = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_options_show_time));
  title_overlay = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_options_title_overlay));
  limit = G15_LCD_HEIGHT - INFERIOR_SPACE*show_pbar - CHAR_HEIGH * (1 - title_overlay) * show_title * rownum - 1;
  pthread_mutex_unlock (&g15buf_mutex);
  return;
}

void g15analyser_conf_reset(void){
  /* Apply default values */
  pthread_mutex_lock (&g15buf_mutex);
  vis_type = def_vis_type;
  num_bars = def_num_bars;
  linearity = def_linearity;
  amplification = def_amplification;
  limit = def_limit;
  enable_peak = def_enable_peak;
  detached_peak = def_detached_peak;
  line_mode = def_line_mode;
  analog_mode = def_analog_mode;
  analog_step = def_analog_step;
  enable_keybindings = def_enable_keybindings;
  show_title = def_show_title;
  show_pbar = def_show_pbar;
  show_time = def_show_time;
  rownum = def_rownum;
  title_overlay = def_title_overlay;
  limit = G15_LCD_HEIGHT - INFERIOR_SPACE*show_pbar - CHAR_HEIGH * (1 - title_overlay) * show_title * rownum- 1;
  pthread_mutex_unlock (&g15buf_mutex);
  return;
}

static void g15analyser_conf_reset_defaults_gui(void){
  /* Apply defaults values to the gui only */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_bars_radio),def_vis_type == 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_scope_radio),def_vis_type == 1);
  
  tmp_bars = def_num_bars; 
  gtk_adjustment_set_value((GtkAdjustment *)adj_bars,tmp_bars);
  
  tmp_lin = def_linearity; 
  gtk_adjustment_set_value((GtkAdjustment *)adj_lin,tmp_lin);
  
  tmp_ampli = def_amplification;
  gtk_adjustment_set_value((GtkAdjustment *)adj_ampli,tmp_ampli);
  
  tmp_rownum = def_rownum;
  gtk_adjustment_set_value((GtkAdjustment *)adj_rownum,tmp_rownum);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_no),    !def_enable_peak && !def_analog_mode && !def_line_mode);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_line),  !def_enable_peak && !def_analog_mode && def_line_mode);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_peak),   def_enable_peak && !def_analog_mode && !def_line_mode);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_analog),!def_enable_peak &&  def_analog_mode && !def_line_mode);
  
  
  tmp_step = def_analog_step;
  gtk_adjustment_set_value((GtkAdjustment *)adj_step,tmp_step);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_enable_dpeak), def_detached_peak);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_enable_keybindings), def_enable_keybindings);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_show_title), def_show_title);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_show_pbar), def_show_pbar);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_show_time), def_show_time);
  return;
}

static void g15analyzer_adj_changed(GtkWidget *w, int *a)
{
  if (a == &tmp_bars){
    /* FIXME: Something better than this ? */
    if (tmp_bars != (int) GTK_ADJUSTMENT(w)->value){
      if ((int) GTK_ADJUSTMENT(w)->value > tmp_bars)
	tmp_bars *= 2;
      else
	tmp_bars /= 2;
    }
    gtk_adjustment_set_value((GtkAdjustment *)w,tmp_bars);
  }    
  else
    *a=(int) GTK_ADJUSTMENT(w)->value;
}

static void g15analyzer_adj_changed_float(GtkWidget *w, float *a)
{
  *a=(float) GTK_ADJUSTMENT(w)->value;
}

void g15analyser_conf_ok(GtkWidget *w, gpointer data){
  /* Ok button routine */
  g15analyser_conf_apply(); /* Apply */
  g15spectrum_write_config(); /* Save Config */
  gtk_widget_destroy(configure_win); /* exit */
  return;
}

void g15analyser_conf_cancel(){
  /* Cancel button routine: */
  g15spectrum_read_config(); /* restore as saved */
  gtk_widget_destroy(configure_win); /* exit */
  return;
}

void g15analyser_conf(void){
  /* some labels */
  GtkWidget *label,*label_bars, *label_lin, *label_ampli, *label_step, *label_rownum;
  tmp_bars = num_bars; /* needed to approx in g15analyzer_adj_changed */
  
  if(configure_win)
    return;
  configure_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width(GTK_CONTAINER(configure_win), 10);
  gtk_window_set_title(GTK_WINDOW(configure_win), PLUGIN_NAME " configuration");
  gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
  gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(configure_win), vbox);
 
  /* general config */
  
  g_options_frame = gtk_frame_new("General:");
  gtk_container_set_border_width(GTK_CONTAINER(g_options_frame), 5);
  gtk_widget_set_size_request (g_options_frame, 258, -1);
  t_options_vistype = gtk_vbox_new(FALSE, 5);
  label = gtk_label_new("Visualization Type:");
  gtk_misc_set_alignment(GTK_MISC (label), 0, 0);
  gtk_misc_set_padding(GTK_MISC (label),5,0);
  gtk_box_pack_start(GTK_BOX(t_options_vistype), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  t_options_bars_radio = gtk_radio_button_new_with_label(NULL, "Spectrum Bars");
  t_options_scope_radio = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(t_options_bars_radio)), "Scope");
  /* Bars radio */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_bars_radio), vis_type == 0);
  gtk_box_pack_start(GTK_BOX(t_options_vistype), t_options_bars_radio, FALSE, FALSE, 0);
  gtk_widget_show(t_options_bars_radio);
  /* Spectrum radio */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_scope_radio), vis_type == 1);
  gtk_box_pack_start(GTK_BOX(t_options_vistype), t_options_scope_radio, FALSE, FALSE, 0);
  gtk_widget_show(t_options_scope_radio);
  /* create keybindings button */
  g_options_enable_keybindings = gtk_check_button_new_with_label("Enable Keybindings (need to restart Audacious)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_enable_keybindings), enable_keybindings);
  /* put check button in g_options_vbox */
  gtk_box_pack_start(GTK_BOX(t_options_vistype), g_options_enable_keybindings, FALSE, FALSE, 0);
  gtk_widget_show(g_options_enable_keybindings);
  /* create show title button */
  g_options_show_title = gtk_check_button_new_with_label("Show title");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_show_title), show_title);
  /* put check button in g_options_vbox */
  gtk_box_pack_start(GTK_BOX(t_options_vistype), g_options_show_title, FALSE, FALSE, 0);
  gtk_widget_show(g_options_show_title);
  /* create title overlay button */
  g_options_title_overlay = gtk_check_button_new_with_label("Title overlay");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_title_overlay), title_overlay);
  /* put check button in g_options_vbox */
  gtk_box_pack_start(GTK_BOX(t_options_vistype), g_options_title_overlay, FALSE, FALSE, 0);
  gtk_widget_show(g_options_title_overlay);
  /* create show progress bar button */
  g_options_show_pbar = gtk_check_button_new_with_label("Show progress bar");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_show_pbar), show_pbar);
  /* put check button in g_options_vbox */
  gtk_box_pack_start(GTK_BOX(t_options_vistype), g_options_show_pbar, FALSE, FALSE, 0);
  gtk_widget_show(g_options_show_pbar);
  /* create show time in progress bar button */
  g_options_show_time = gtk_check_button_new_with_label("Show time in progress bar");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_show_time), show_time);
  /* put check button in g_options_vbox */
  gtk_box_pack_start(GTK_BOX(t_options_vistype), g_options_show_time, FALSE, FALSE, 0);
  gtk_widget_show(g_options_show_time);
  /* Num lines bar */
  label_rownum=gtk_label_new("Split title in lines (1 = cycle):");
  gtk_misc_set_alignment(GTK_MISC (label_rownum), 0, 0);
  gtk_misc_set_padding(GTK_MISC (label_rownum),5,0);
  gtk_box_pack_start(GTK_BOX(t_options_vistype), label_rownum, TRUE, TRUE, 4);
  gtk_widget_show(label_rownum);
  adj_rownum=gtk_adjustment_new(rownum, 1, 4, 1, 1, 0);
  scale_rownum=gtk_hscale_new(GTK_ADJUSTMENT(adj_rownum));
  gtk_scale_set_draw_value(GTK_SCALE(scale_rownum), TRUE);
  gtk_scale_set_digits ((GtkScale *)scale_rownum, 0);
  gtk_widget_show(scale_rownum);
  gtk_box_pack_start(GTK_BOX(t_options_vistype), scale_rownum, TRUE, TRUE, 4);
  gtk_signal_connect(GTK_OBJECT(adj_rownum), "value-changed", GTK_SIGNAL_FUNC(g15analyzer_adj_changed), &tmp_rownum);
  
  /* draw frame */
  gtk_container_add(GTK_CONTAINER(g_options_frame), t_options_vistype); 
  gtk_widget_show(t_options_vistype);
  gtk_box_pack_start(GTK_BOX(vbox), g_options_frame, TRUE, TRUE, 0);
  gtk_widget_show(g_options_frame);
  
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(hbox);
  //gtk_container_add(GTK_BOX (vbox), hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  /* bars config */
  
  g_options_frame_bars = gtk_frame_new("Spectrum bars options:");
  gtk_container_set_border_width(GTK_CONTAINER(g_options_frame), 5);
  //gtk_widget_set_size_request (g_options_frame_bars, 258, 258);
  t_options_bars = gtk_vbox_new(FALSE, 5);
  /* number of the bars */
  label_bars=gtk_label_new("Num Bars:");
  gtk_misc_set_alignment(GTK_MISC (label_bars), 0, 0);
  gtk_misc_set_padding(GTK_MISC (label_bars),5,0);
  gtk_box_pack_start(GTK_BOX(t_options_bars), label_bars, TRUE, TRUE, 4);
  gtk_widget_show(label_bars);
  adj_bars=gtk_adjustment_new(num_bars, 2, 128, 2, 2, 0);
  scale_bars=gtk_hscale_new(GTK_ADJUSTMENT(adj_bars));
  gtk_scale_set_draw_value(GTK_SCALE(scale_bars), TRUE);
  gtk_scale_set_digits ((GtkScale *)scale_bars, 0);
  gtk_widget_show(scale_bars);
  gtk_box_pack_start(GTK_BOX(t_options_bars), scale_bars, TRUE, TRUE, 4);
  gtk_signal_connect(GTK_OBJECT(adj_bars), "value-changed", GTK_SIGNAL_FUNC(g15analyzer_adj_changed), &tmp_bars);
  /* Amplification linearity bar */
  label_lin=gtk_label_new("Amplification linearity:");
  gtk_misc_set_alignment(GTK_MISC (label_lin), 0, 5);
  gtk_misc_set_padding(GTK_MISC (label_lin),5,0);
  gtk_box_pack_start(GTK_BOX(t_options_bars), label_lin, TRUE, TRUE, 4);
  gtk_widget_show(label_lin);
  adj_lin=gtk_adjustment_new(linearity, 0.1 , 0.49 , 0.01, 0.01, 0);
  scale_lin=gtk_hscale_new(GTK_ADJUSTMENT(adj_lin));
  gtk_scale_set_digits ((GtkScale *)scale_lin, 2);
  gtk_scale_set_draw_value(GTK_SCALE(scale_lin), TRUE);
  gtk_widget_show(scale_lin);
  gtk_box_pack_start(GTK_BOX(t_options_bars), scale_lin, TRUE, TRUE, 4);
  gtk_signal_connect(GTK_OBJECT(adj_lin), "value-changed", GTK_SIGNAL_FUNC(g15analyzer_adj_changed_float), &tmp_lin);
  /* Amplification bar */
  label_ampli=gtk_label_new("Amplification:");
  gtk_misc_set_alignment(GTK_MISC (label_ampli), 0, 5);
  gtk_misc_set_padding(GTK_MISC (label_ampli),5,0);
  gtk_box_pack_start(GTK_BOX(t_options_bars), label_ampli, TRUE, TRUE, 4);
  gtk_widget_show(label_ampli);
  adj_ampli=gtk_adjustment_new(amplification, -30, 30, 1, 1, 0);
  scale_ampli=gtk_hscale_new(GTK_ADJUSTMENT(adj_ampli));
  gtk_scale_set_draw_value(GTK_SCALE(scale_ampli), TRUE);
  gtk_scale_set_digits ((GtkScale *)scale_ampli, 0);
  gtk_widget_show(scale_ampli);
  gtk_box_pack_start(GTK_BOX(t_options_bars), scale_ampli, TRUE, TRUE, 4);
  gtk_signal_connect(GTK_OBJECT(adj_ampli), "value-changed", GTK_SIGNAL_FUNC(g15analyzer_adj_changed), &tmp_ampli);
  /* draw frame */
  gtk_container_add(GTK_CONTAINER(g_options_frame_bars), t_options_bars); 
  gtk_widget_show(t_options_bars);
  gtk_box_pack_start(GTK_BOX(hbox), g_options_frame_bars, TRUE, TRUE, 0);
  gtk_widget_show(g_options_frame_bars);
  
  
  /* bars effects */
  g_options_frame_bars_effects = gtk_frame_new("Spectrum bars effects:");
  gtk_container_set_border_width(GTK_CONTAINER(g_options_frame_bars_effects), 5);
  //gtk_widget_set_size_request (g_options_frame_bars_effects, 258, 268);
  t_options_bars_effects = gtk_vbox_new(FALSE, 5);
  /* radio effect type */
  t_options_effect_no     = gtk_radio_button_new_with_label(NULL, "None");
  t_options_effect_line   = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(t_options_effect_no)), "Line");
  t_options_effect_peak   = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(t_options_effect_line)), "Peaks");
  t_options_effect_analog = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(t_options_effect_peak)), "Analog");
  /* no effect radio */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_no), (enable_peak == FALSE && analog_mode == FALSE && line_mode == FALSE));
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), t_options_effect_no, FALSE, FALSE, 0);
  gtk_widget_show(t_options_effect_no);
  /* line effect radio */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_line), (enable_peak == FALSE && analog_mode == FALSE && line_mode == TRUE));
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), t_options_effect_line, FALSE, FALSE, 0);
  gtk_widget_show(t_options_effect_line);
  /* peak effect radio */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_peak), (enable_peak == TRUE && analog_mode == FALSE && line_mode == FALSE));
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), t_options_effect_peak, FALSE, FALSE, 0);
  gtk_widget_show(t_options_effect_peak);
  /* analog effect radio */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t_options_effect_analog), (enable_peak == FALSE && analog_mode == TRUE && line_mode == FALSE));
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), t_options_effect_analog, FALSE, FALSE, 0);
  gtk_widget_show(t_options_effect_analog);
  
  /* peak detached button */
  g_options_enable_dpeak = gtk_check_button_new_with_label("Detach peaks from bars");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_options_enable_dpeak), detached_peak);
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), g_options_enable_dpeak, FALSE, FALSE, 0);
  gtk_widget_show(g_options_enable_dpeak);
  
  /* analog step bar */
  label_step=gtk_label_new("Analog mode step:");
  gtk_misc_set_alignment(GTK_MISC (label_step), 0, 5);
  gtk_misc_set_padding(GTK_MISC (label_step),5,0);
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), label_step, TRUE, TRUE, 4);
  gtk_widget_show(label_step);
  adj_step=gtk_adjustment_new(analog_step, 2, 9, 1, 1, 0);
  scale_step=gtk_hscale_new(GTK_ADJUSTMENT(adj_step));
  gtk_scale_set_draw_value(GTK_SCALE(scale_step), TRUE);
  gtk_scale_set_digits ((GtkScale *)scale_step, 0);	
  gtk_widget_show(scale_step);
  gtk_box_pack_start(GTK_BOX(t_options_bars_effects), scale_step, TRUE, TRUE, 4);
  gtk_signal_connect(GTK_OBJECT(adj_step), "value-changed", GTK_SIGNAL_FUNC(g15analyzer_adj_changed), &tmp_step);
  
  /* draw frame */
  gtk_container_add(GTK_CONTAINER(g_options_frame_bars_effects), t_options_bars_effects); 
  gtk_widget_show(t_options_bars_effects);
  gtk_box_pack_start(GTK_BOX(hbox), g_options_frame_bars_effects, TRUE, TRUE, 0);
  gtk_widget_show(g_options_frame_bars_effects);
  
  
  /* buttons */
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);   
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
  
  ok = gtk_button_new_from_stock ("gtk-save");
  gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(g15analyser_conf_ok), NULL);
  GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
  gtk_widget_show(ok);
  
  apply = gtk_button_new_from_stock ("gtk-apply");
  gtk_signal_connect(GTK_OBJECT(apply), "clicked", GTK_SIGNAL_FUNC(g15analyser_conf_apply), NULL);
  GTK_WIDGET_SET_FLAGS(apply, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), apply, TRUE, TRUE, 0);
  gtk_widget_show(apply);
  
  defaults = gtk_button_new_from_stock ("gtk-revert-to-saved");
  gtk_signal_connect(GTK_OBJECT(defaults), "clicked", GTK_SIGNAL_FUNC(g15analyser_conf_reset_defaults_gui), NULL);
  GTK_WIDGET_SET_FLAGS(defaults, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), defaults, TRUE, TRUE, 0);
  gtk_widget_show(defaults);
  
  cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(g15analyser_conf_cancel), GTK_OBJECT(configure_win));
  GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
  
  /* Show all */
  gtk_widget_show(cancel);
  gtk_widget_show(bbox);

  gtk_widget_show(configure_win);
  return;
}

void g15analyser_about(void){
  GtkWidget *dialog, *button, *label;
  
  dialog = gtk_dialog_new();
  
  gtk_widget_set_usize (dialog, 400, 300);
  gtk_window_set_title(GTK_WINDOW(dialog), "about " PLUGIN_NAME);
  gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, FALSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 5);
  
  /* Something about us... */
  label = gtk_label_new (PLUGIN_NAME"\n\
      v. " PLUGIN_VERSION "\n	       \
      \n					\
      by Mike Lampard <mlampard@users.sf.net>\n	\
      Anthony J. Mirabella <aneurysm9>\n		\
      Antonio Bartolini <robynhub@users.sf.net>\n	\
      and others...\n					\
      \n						\
      get the newest version from:\n			\
      http://g15daemon.sf.net/\n			\
      ");
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  
  button = gtk_button_new_with_label(" Ok ");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
  
  gtk_widget_show(dialog);
  gtk_widget_grab_focus(button);
  
  return;
}


static int poll_g15keys() {
  int keystate = 0;
  int ret;
  struct pollfd fds;
  
  fds.fd = g15screen_fd;
  fds.events = POLLIN;
  
  if ((poll(&fds, 1, 5)) > 0)
    ret = read(g15screen_fd, &keystate, sizeof (keystate));
  
  if (keystate) {
    pthread_mutex_lock (&g15buf_mutex);
    switch (keystate)
      {
      case G15_KEY_L1:
	vis_type = 1 - vis_type;
	g15spectrum_write_config(); /* save as default next time */
	break;
      default:
	break;
      }
    keystate = 0;
    pthread_mutex_unlock (&g15buf_mutex);
  }
  return TRUE;
}

static int poll_mmediakeys()
{
  long mask = KeyPressMask;
  XEvent event;
  
  while (XCheckMaskEvent(dpy, mask, &event)){
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioPlay)) {
      if(playing) {
	if (paused)  {
#ifdef OLD_PLUGIN
	  xmms_remote_play(g15analyser_vp.xmms_session);
#else
	  audacious_drct_play();
#endif
	  paused = 0;
	} else {
#ifdef OLD_PLUGIN
	  xmms_remote_pause(g15analyser_vp.xmms_session);
#else
	  audacious_drct_pause();
#endif
	  paused = 1;
	}
      } else
#ifdef OLD_PLUGIN	
	xmms_remote_play(g15analyser_vp.xmms_session);
#else
      audacious_drct_pause();
#endif
    }
    
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioStop))
#ifdef OLD_PLUGIN 
      xmms_remote_stop(g15analyser_vp.xmms_session);
#else
    audacious_drct_stop();
#endif
    
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioLowerVolume)){
      volume = get_main_volume();
      if(volume<1)
	volume=1;
#ifdef OLD_PLUGIN
      xmms_remote_set_main_volume(g15analyser_vp.xmms_session, --volume);
#else
      audacious_drct_set_main_volume(--volume);
#endif 
    }
    
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioRaiseVolume)){
      volume = get_main_volume();
      if(volume>99)
	volume=99;
#ifdef OLD_PLUGIN
      xmms_remote_set_main_volume(g15analyser_vp.xmms_session, ++volume);
#else
      audacious_drct_set_main_volume(++volume);
#endif
    }
    
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioMute)){
      if(get_main_volume()!=0){
	lastvolume = get_main_volume();
	volume = 0;
      }
      else {
	volume = lastvolume;
      }
#ifdef OLD_PLUGIN   
      xmms_remote_set_main_volume(g15analyser_vp.xmms_session, volume);
#else
      audacious_drct_set_main_volume(volume);
#endif
    }
    
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioNext))
      if (playing)
#ifdef OLD_PLUGIN
	xmms_remote_playlist_next(g15analyser_vp.xmms_session);
#else
    audacious_drct_playlist_next();
#endif
    
    if(event.xkey.keycode==XKeysymToKeycode(dpy, XF86XK_AudioPrev))
      if (playing)
#ifdef OLD_PLUGIN
	xmms_remote_playlist_prev(g15analyser_vp.xmms_session);
#else
    audacious_drct_playlist_prev();
#endif
  }
  return TRUE;
}

static int g15send_func() {
  int i,j;
  int playlist_pos;
  char *title;
  char *strtok_ptr;
  /* total length of the string */
  int rendstrlen = (ROWLEN * rownum); 
  static int vol_timeout=0;
  long chksum=0;
  static long last_chksum;
  
  pthread_mutex_lock (&g15buf_mutex);
  g15r_clearScreen (canvas, G15_COLOR_WHITE);
  
#ifdef OLD_PLUGIN  
  if (xmms_remote_get_playlist_length(g15analyser_vp.xmms_session) > 0){
    playlist_pos = xmms_remote_get_playlist_pos(g15analyser_vp.xmms_session);
    
    title = xmms_remote_get_playlist_title(g15analyser_vp.xmms_session, playlist_pos);
#else
    if (audacious_drct_get_playlist_length() > 0){
      playlist_pos = audacious_drct_get_playlist_pos();
      title = audacious_drct_get_playlist_title(playlist_pos);
#endif 
      if(title!=NULL && show_title){
	if (rownum != 1) {
	  /*
	    amarok doesnt support providing title info via xmms interface
	    
	    print multiple line with automatic or forced with 
	    SEPARATOR char newline. Thanks to Giuseppe della Bianca.
	    
	    TODO: gtk interface
	  */ 
	  int NumRow, TokenEnd, TokenLen, SepFind;
	  char RendStr[rendstrlen + 1], RendToken[rendstrlen + 1];
	  char *pRendStr, *pRendToken;
	  
	  strncpy (RendStr, title, rendstrlen);
	  if (strlen (title) >= rendstrlen)
	    RendStr[rendstrlen]= '\0';
	  pRendStr= RendStr;
	  for (NumRow= 0; NumRow < rownum; NumRow++){
	    strcpy (RendToken, pRendStr);
	    pRendToken= strtok_r (RendToken, SEPARATOR , &strtok_ptr);
	    if (pRendToken == NULL){
	      strcpy (RendToken, "");
	      pRendToken= RendToken;
	    }
	    TokenEnd= TokenLen= strlen (pRendToken);
	    
	    /* if is find the '-' char */
	    SepFind= FALSE;
	    if (strcmp (pRendToken, pRendStr)){
	      SepFind= TRUE;
	      /* remove space char */
	      if (TokenLen > 0){
		if (pRendToken[TokenLen - 1] == ' ')
		  pRendToken[TokenLen - 1]= '\0';
		if (*pRendToken == ' ')
		  pRendToken++;
	      }
	      TokenEnd++;
	    }
	    /* automatic new line */
	    TokenLen= strlen (pRendToken);
	    if (TokenLen > 0){
	      if (TokenLen > ROWLEN){
		TokenLen= ROWLEN;
		TokenEnd= (int)(pRendToken - RendToken) + TokenLen;
		pRendToken[TokenLen]= '\0';
	      }
	      
	      
	      
	      if (*pRendToken != '\0')
		/* g15r_renderString (canvas, (unsigned char *)pRendToken, 0, FONT_SIZE, 0, NumRow * CHAR_HEIGH); */
	      g15r_G15FPrint (canvas, pRendToken, 0, NumRow * CHAR_HEIGH, FONT_SIZE, 2,G15_COLOR_BLACK, 0);
	    }
	    
	    pRendStr= &pRendStr[TokenEnd];
	    if (SepFind){
	      /* remove space char */
	      if (*pRendStr == ' ')
		pRendStr++;
	    }
	  }
	} else{
	  /* Only one line */
	  int title_pixel = g15r_testG15FontWidth(font,title);
	  int i;
	  // Substitution "_" with " "
	  for (i = 0 ; i < strlen(title); i++){
	    if (title[i] == '_')
	      title[i] = ' ';
	  }
	  if (title_pixel <= G15_LCD_WIDTH){
	    
	    /* g15r_renderString (canvas, (unsigned char *)title, 0, FONT_SIZE, 0, 0); */
	    g15r_G15FPrint (canvas, title, 0, 0, FONT_SIZE, 2,G15_COLOR_BLACK, 0);
	  } else {
	    /* title cycle :D */
	    /* roll it over my soul... (Oasis) */
	    text_start++;
	    text_start2++;
	    
	    if (text_start > text_start2){
	      /* Text 1 first */
	      text_start2 = text_start - title_pixel - 25 % (title_pixel + G15_LCD_WIDTH);
	      text_start = text_start % (title_pixel + G15_LCD_WIDTH);
	    } else {
	      /* Text 2 first */
	      text_start =  text_start2 - title_pixel - 25 % (title_pixel + G15_LCD_WIDTH);
	      text_start2 = text_start2 % (title_pixel + G15_LCD_WIDTH);
	    }
	    g15r_renderString (canvas, (unsigned char *)title, 0, FONT_SIZE, G15_LCD_WIDTH - text_start2, 0);
	    g15r_renderString (canvas, (unsigned char *)title, 0, FONT_SIZE, G15_LCD_WIDTH - text_start, 0);
	  }
	}
      }
      if (show_pbar){
#ifdef OLD_PLUGIN
	int output_time = xmms_remote_get_output_time(g15analyser_vp.xmms_session)/1000;
	int playlist_time = xmms_remote_get_playlist_time(g15analyser_vp.xmms_session,playlist_pos)/1000;
#else
	int output_time = audacious_drct_get_output_time()/1000;
	int playlist_time = audacious_drct_get_playlist_time(playlist_pos)/1000;
#endif
	
	/* bugfix: Sometimes xmms don't get the output time */
	if (playlist_time == 0){
	  playlist_time = 0;
	  output_time = 0;
	  
	}
	if (show_time){
	  unsigned char time[10];
	  unsigned char endtime[10];
	  
	  snprintf((char *)time,10,"%02d:%02d",output_time/60,output_time%60);
	  snprintf((char *)endtime,10,"%02d:%02d",playlist_time/60,playlist_time%60);
	  if (playlist_time != 0)
	    g15r_drawBar (canvas, 20, 39, 138, 41, G15_COLOR_BLACK, output_time, playlist_time, 1);
	  else 
	   g15r_drawBar (canvas, 20, 39, 138, 41, G15_COLOR_BLACK, 0, 1, 1); 
	  g15r_renderString (canvas, time, 0, 5, 0, 37);
	  g15r_renderString (canvas, endtime, 0, 5, 140, 37);
	} else 
	  g15r_drawBar (canvas, 0, 39, 159, 41, G15_COLOR_BLACK, output_time, playlist_time, 1); 
      }
      
      if (playing)
	{
	  if (vis_type == 0)
	    {
	      int bar_width =  G15_LCD_WIDTH / num_bars;
	      int bottom_line = G15_LCD_HEIGHT-(INFERIOR_SPACE*show_pbar); /* line where bars starts */
	      
	      if (!show_pbar) bottom_line--;  /* we don't draw pixel 0 */
	      
	      for(i = 0; i < G15_LCD_WIDTH; i+=bar_width){
		int y1 = bar_heights[i];
		/* check bars length anyway */
		if (y1 > limit)
		  y1 = limit;
		if (y1 < 0)
		  y1 = 0;
		
		/* if enable peak... calculate it */
		if (enable_peak && ! analog_mode){
		  if (y1 > limit - 2)   /* check new limit for bars to show peaks even when max */
		    y1 = limit - 2;
		  /* check for new peak */
		  if(y1 > bar_heights_peak[i]) {          
		    bar_heights_peak[i] = y1;
		  } else 
		    /* decrement old peak */
		    bar_heights_peak[i]--;              
		} 
		
		y1 = bottom_line - y1; /* always show bottom of the bars */
		
		/* Analog Mode */
		if (analog_mode && ! line_mode){
		  int end =  y1  + analog_step - (y1 % analog_step) - 1; /* superior approx to multiple */
		  for(j = bottom_line ; j >= end; j-= analog_step){
		    /* draw leds :-) */
		    g15r_pixelBox (canvas, i, j - analog_step + 2 ,i + bar_width - 2, j , G15_COLOR_BLACK, 1, 1); 
		  }		  
		} else {
		  /* line mode */
		  if (line_mode)
		    g15r_drawLine (canvas, i, y1 , i + bar_width - 1, y1, G15_COLOR_BLACK);
		  else
		    g15r_pixelBox (canvas, i, y1 , i + bar_width - 2, bottom_line, G15_COLOR_BLACK, 1, 1);
		  
		}
		/* Enable peak*/
		if (enable_peak && ! analog_mode && ! line_mode){        
		  int peak;
		  /* superior limit */
		  if (bar_heights_peak[i] < limit){     
		    peak = bottom_line - bar_heights_peak[i] - detached_peak*2; /* height of peak */
		    
		    /* inferior limit */
		    if ( bar_heights_peak[i] > 0)                    
		      g15r_pixelBox (canvas, i, peak  , i + bar_width - 2, peak, G15_COLOR_BLACK, 1, 1);
		  }
		  
		}
	      }
	    }
	  else
	    {
	      /* render scope */
	      int y1, y2=(25 - scope_data[0]);
	      for (i = 0; i < G15_LCD_WIDTH; i++)
		{
		  y1 = y2;
		  y2 = (25 - scope_data[i]);
		  g15r_drawLine (canvas, i, y1, i + 1, y2, G15_COLOR_BLACK);
		}
	    }
	}
      else 
	/* g15r_renderString (canvas, (unsigned char *)"Playback Stopped", 0, 13, 21, 16); */
	g15r_G15FPrint (canvas, "Playback Stopped", 21, 16, 13, 1, G15_COLOR_BLACK, 0);
      
    }
    else
      /* g15r_renderString (canvas, (unsigned char *)"Playlist Empty", 0, 13, 34, 16); */
      g15r_G15FPrint (canvas, "Playlist Empty", 34, 16, 13, 1, G15_COLOR_BLACK, 0);
    
    if(lastvolume!=get_main_volume() || vol_timeout>=0) {
      if(lastvolume!=get_main_volume())
	vol_timeout=10;
      else
	vol_timeout--;
      /* render volume */
      lastvolume = get_main_volume();
      if (lastvolume >= 0)	
	g15r_drawBar (canvas, 10, 15, 149, 28, G15_COLOR_BLACK, lastvolume, 100, 1);
      canvas->mode_xor=1;
      g15r_renderString (canvas, (unsigned char *)"Volume", 0, 12, 59, 17);
      canvas->mode_xor=0;
    }
    /* do a quicky checksum - only send frame if different */
    for(i=0;i<G15_BUFFER_LEN;i++){
      chksum+=canvas->buffer[i]*i;
    }
    if(last_chksum!=chksum) {
      if(g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN)<0) {
	perror("lost connection, tryng again\n");
	/* connection error occurred - try to reconnect to the daemon */
	g15screen_fd=new_g15_screen(G15_G15RBUF);
      }
    }
    last_chksum=chksum;
    
    pthread_mutex_unlock(&g15buf_mutex);
    return TRUE;
  }
  
  int myx_error_handler(Display *dpy, XErrorEvent *err){
    
    printf("error (%i) occured - ignoring\n",err->error_code);
    return 0;
  }
  
  
  static void g15analyser_init(void) {
    
    pthread_mutex_init(&g15buf_mutex, NULL); 
    g15analyser_conf_reset();
    g15spectrum_read_config();
    
    pthread_mutex_lock(&g15buf_mutex);
    
    if (enable_keybindings) {
      dpy = XOpenDisplay(getenv("DISPLAY"));
      if (!dpy) {
	printf("Can't open display\n");
	return;
      }
      root_win = DefaultRootWindow(dpy);
      if (!root_win) {
	printf("Cant find root window\n");
	return;
      }
      
      /* completely ignore errors and carry on */
      XSetErrorHandler(myx_error_handler);
      XFlush(dpy);
      
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioPlay), AnyModifier, root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioStop), AnyModifier, root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioPrev), AnyModifier, root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioNext), AnyModifier, root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioLowerVolume),Mod2Mask , root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioRaiseVolume), Mod2Mask, root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
      XGrabKey(dpy,XKeysymToKeycode(dpy, XF86XK_AudioMute), Mod2Mask, root_win,
	       False, GrabModeAsync, GrabModeAsync);                                     
    }
    
    g15screen_fd = new_g15_screen(G15_G15RBUF);
    if(g15screen_fd < 0 ){
      printf("Cant connect with G15daemon !\n");
      pthread_mutex_unlock(&g15buf_mutex);
      gtk_idle_add (g15analyser_disable,NULL);
      return;
    }
    canvas = (g15canvas *) malloc (sizeof (g15canvas));
    if (canvas != NULL)
      {
	memset(canvas->buffer, 0, G15_BUFFER_LEN);
	canvas->mode_cache = 0;
	canvas->mode_reverse = 0;
	canvas->mode_xor = 0;
      }
    
    /* requesting font size needed */ 
    font = g15r_requestG15DefaultFont(FONT_SIZE);
    pthread_mutex_unlock(&g15buf_mutex);
    /* increase lcd drive voltage/contrast for this client */
    g15_send_cmd(g15screen_fd, G15DAEMON_CONTRAST,2);
    
    pthread_attr_t attr;
    memset(&attr,0,sizeof(pthread_attr_t));
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if (enable_keybindings){
      mmedia_timeout_handle = g_timeout_add(100, poll_mmediakeys, NULL);
      g15keys_timeout_handle = g_timeout_add(100, poll_g15keys, NULL);
    }
    g15disp_timeout_handle = g_timeout_add(75, g15send_func, NULL);
    
  }
  
  static void g15analyser_cleanup(void) {
    
    pthread_mutex_lock (&g15buf_mutex);
    /* Write down config */
    g15spectrum_write_config();
    if (canvas != NULL)
      free(canvas);
    if(g15screen_fd)
      close(g15screen_fd);
    pthread_mutex_unlock (&g15buf_mutex);
    if (enable_keybindings){
      XUngrabKey(dpy, XF86XK_AudioPrev, AnyModifier, root_win);
      XUngrabKey(dpy, XF86XK_AudioNext, AnyModifier, root_win);
      XUngrabKey(dpy, XF86XK_AudioPlay, AnyModifier, root_win);
      XUngrabKey(dpy, XF86XK_AudioStop, AnyModifier, root_win);
      XUngrabKey(dpy, XF86XK_AudioLowerVolume, Mod2Mask, root_win);
      XUngrabKey(dpy, XF86XK_AudioRaiseVolume, Mod2Mask, root_win);
      XUngrabKey(dpy, XF86XK_AudioMute, AnyModifier, root_win);
      
      gtk_timeout_remove(mmedia_timeout_handle);
      gtk_timeout_remove(g15keys_timeout_handle);
    }
    gtk_timeout_remove(g15disp_timeout_handle);
    if (enable_keybindings)
      XCloseDisplay(dpy);
    
    return;
  }
  
  static void g15analyser_playback_start(void) {
    
    pthread_mutex_lock (&g15buf_mutex);
    playing = 1;
    paused = 0;
    pthread_mutex_unlock (&g15buf_mutex);
    return;
    
  }
  
  static void g15analyser_playback_stop(void) {
    
    pthread_mutex_lock (&g15buf_mutex);
    playing = 0;
    paused = 0;
    pthread_mutex_unlock (&g15buf_mutex);
    return;
    
  }
  
  static void g15analyser_render_pcm(gint16 data[2][512]) {
    pthread_mutex_lock (&g15buf_mutex);
    
    if (playing)
      {
	gint i;
	gint max;
	gint scale = 128;
	
	do
	  {
	    max = 0;
	    for (i = 0; i < G15_LCD_WIDTH; i++)
	      {
		scope_data[i] = data[0][i] / scale;  /* FIXME: Use both channels? */
		if (abs(scope_data[i]) > abs(max))
		  max = scope_data[i];
	      }
	    scale += 128;
	  } while (abs(max) > 10);
      }
    
    pthread_mutex_unlock (&g15buf_mutex);
  }
  
  static void g15analyser_render_freq(gint16 data[2][256]) {
    
    
    pthread_mutex_lock(&g15buf_mutex);
    
    if (playing)
      {
	gint i,drawable_space;
	gdouble y;
	/* code from version 0.1 */
	
	/* dynamically calculate the scale (maybe changed in config) */
	drawable_space = G15_LCD_HEIGHT - INFERIOR_SPACE*show_pbar - CHAR_HEIGH*(1-title_overlay)*show_title*rownum + amplification;
	scale = drawable_space / ( log((1 - linearity) / linearity) *2 );  
	x00 = linearity*linearity*32768.0/(2 * linearity - 1);
	y00 = -log(-x00) * scale;
	
	for (i = 0; i < WIDTH; i++) {
	  y = (gdouble)data[0][i] * (i + 1); /* Compensating the energy */  /* FIXME: Use both channels? */
	  y = ( log(y - x00) * scale + y00 ); /* Logarithmic amplitude */
	  
	  y = ( (dif-2)*y + /* FIXME: conditionals should be rolled out of the loop */
		(i==0       ? y : bar_heights[i-1]) +
		(i==WIDTH-1 ? y : bar_heights[i+1])) / dif; /* Add some diffusion */
	  y = ((tau-1)*bar_heights[i] + y) / tau; /* Add some dynamics */
	  bar_heights[i] = (gint16)y; 
	}
	
      }
    
    pthread_mutex_unlock(&g15buf_mutex);
    
    return;
    
  }
  
