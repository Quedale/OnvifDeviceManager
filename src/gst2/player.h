#include "gst/gst.h"
#include "onvif_list.h"
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video.h>
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
  GstElement *pipeline; /* Our one and only pipeline */
  GstElement *backpipe;
  GstElement *src;  /* RtspSrc to support backchannel */
  GstElement *sink;  /* Video Sink */
  GstVideoOverlay *overlay; //Overlay rendered on the canvas widget

  GstState state;                 /* Current state of the pipeline */
  OnvifDevice* device; /* Currently selected device */
  OnvifDeviceList* onvifDeviceList;
  GtkWidget *listbox;
  GtkWidget *details_notebook;
  GtkWidget *canvas;
  guintptr video_window_handle;
  gdouble level; //Used to calculate level decay
  guint back_stream_id;

  //Cairo overlay
  int valid;//For some reason taking this out causes a segment fault, although there's no reference to it???
  int width;
  int height;
  pthread_mutex_t * player_lock;
} OnvifPlayer;

OnvifPlayer * OnvifPlayer__create();  // equivalent to "new Point(x, y)"
void OnvifPlayer__destroy(OnvifPlayer* self);  // equivalent to "delete point"
void OnvifPlayer__set_playback_url(OnvifPlayer* self, char *url);
void OnvifPlayer__stop(OnvifPlayer* self);
void OnvifPlayer__play(OnvifPlayer* self);
GtkWidget * OnvifDevice__createCanvas(OnvifPlayer *self);

#endif