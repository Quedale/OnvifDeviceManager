
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gio/gio.h>

#ifndef ONVIF_PLAYER_OVERLAY_H_ 
#define ONVIF_PLAYER_OVERLAY_H_

typedef struct
{
  gboolean valid;
  GstVideoInfo info;
} OverlayState;

void prepare_overlay (GstElement * overlay, GstCaps * caps, gint window_width, gint window_height, gpointer user_data);

GstVideoOverlayComposition * draw_overlay (GstElement * overlay, GstSample * sample, gpointer user_data);

#endif