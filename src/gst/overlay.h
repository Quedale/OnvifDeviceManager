#ifndef ONVIF_PLAYER_OVERLAY_H_ 
#define ONVIF_PLAYER_OVERLAY_H_

#include <gst/video/video.h>

typedef struct _OverlayState OverlayState;

OverlayState * OverlayState__create();
void OverlayState__init(OverlayState * self);
void OverlayState__destroy(OverlayState * self);

void OverlayState__prepare_overlay (GstElement * overlay, GstCaps * caps, gint window_width, gint window_height, gpointer user_data);
GstVideoOverlayComposition * OverlayState__draw_overlay (GstElement * overlay, GstSample * sample, gpointer user_data);
void OverlayState__level_handler(GstBus * bus, GstMessage * message, OverlayState *self, const GstStructure *s);

#endif