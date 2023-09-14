#ifndef ONVIF_PLAYER_H_ 
#define ONVIF_PLAYER_H_

#include <gtk/gtk.h>
#include "overlay.h"


typedef struct _RtspPlayer RtspPlayer;

typedef struct _RtspPlayer {
  GstElement *pipeline;
  GstElement *src;  /* RtspSrc to support backchannel */
  GstElement *sink;  /* Video Sink */
  int pad_found;
  GstElement * video_bin;
  int no_video;
  int video_done;
  GstElement * audio_bin;
  int no_audio;
  int audio_done;
  
  char * location;
  //Backpipe related properties
  int enable_backchannel;
  GstElement *backpipe;
  GstElement *mic_volume_element;
  GstElement *appsink;
  char * mic_element;
  char * mic_device;

  GstVideoOverlay *overlay; //Overlay rendered on the canvas widget
  OverlayState *overlay_state;

  int retry;
  int playing;

  void (*retry_callback)(RtspPlayer *, void * user_data);
  void * retry_user_data;

  void (*error_callback)(RtspPlayer *, void * user_data);
  void * error_user_data;

  void (*stopped_callback)(RtspPlayer *, void * user_data);
  void * stopped_user_data;

  void (*start_callback)(RtspPlayer *, void * user_data);
  void * start_user_data;

  GtkWidget *canvas_handle;
  GtkWidget *canvas;
  gdouble level; //Used to calculate level decay
  guint back_stream_id;
  int allow_overscale;
  
  pthread_mutex_t * player_lock;
} RtspPlayer;

RtspPlayer * RtspPlayer__create();  // equivalent to "new Point(x, y)"
void RtspPlayer__destroy(RtspPlayer* self);  // equivalent to "delete point"
void RtspPlayer__set_retry_callback(RtspPlayer* self, void (*retry_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__set_error_callback(RtspPlayer* self, void (*error_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__set_stopped_callback(RtspPlayer* self, void (*stopped_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__set_start_callback(RtspPlayer* self, void (*start_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__allow_overscale(RtspPlayer * self, int allow_overscale);
void RtspPlayer__set_playback_url(RtspPlayer* self, char *url);
void RtspPlayer__stop(RtspPlayer* self);
void RtspPlayer__play(RtspPlayer* self);

/*
Compared to play, retry is design to work after a stream failure.
Stopping will essentially break the retry method and stop the loop.
*/
void RtspPlayer__retry(RtspPlayer* self);
GtkWidget * OnvifDevice__createCanvas(RtspPlayer *self);
gboolean RtspPlayer__is_mic_mute(RtspPlayer* self);
void RtspPlayer__mic_mute(RtspPlayer* self, gboolean mute);

#endif