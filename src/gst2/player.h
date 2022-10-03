#include "gst/gst.h"
#include "../ui/onvif_list.h"
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif


#ifndef ONVIF_PLAYER_H_ 
#define ONVIF_PLAYER_H_

/* Structure to contain all our information, so we can pass it around */
typedef struct _OnvifPlayer {
  GstElement *playbin;           /* Our one and only pipeline */
  GstState state;                 /* Current state of the pipeline */
  OnvifDeviceList* onvifDeviceList;
  GtkWidget *listbox;

} OnvifPlayer;

OnvifPlayer OnvifPlayer__create();  // equivalent to "new Point(x, y)"
void OnvifPlayer__destroy(OnvifPlayer* self);  // equivalent to "delete point"
void OnvifPlayer__set_playback_url(OnvifPlayer* self, char *url);
void OnvifPlayer__play(OnvifPlayer* self);
void OnvifPlayer__set_canvas(OnvifPlayer* self, GtkWidget* canvas);

#endif