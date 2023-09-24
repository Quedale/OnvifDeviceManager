#ifndef ONVIF_PLAYER_H_ 
#define ONVIF_PLAYER_H_

#include <gtk/gtk.h>
#include "overlay.h"
#include "backchannel.h"
#include "portable_thread.h"


typedef struct _RtspPlayer RtspPlayer;

RtspPlayer * RtspPlayer__create();  // equivalent to "new Point(x, y)"
void RtspPlayer__destroy(RtspPlayer* self);  // equivalent to "delete point"
void RtspPlayer__set_retry_callback(RtspPlayer* self, void (*retry_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__set_error_callback(RtspPlayer* self, void (*error_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__set_stopped_callback(RtspPlayer* self, void (*stopped_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__set_start_callback(RtspPlayer* self, void (*start_callback)(RtspPlayer *, void *), void * user_data);
void RtspPlayer__allow_overscale(RtspPlayer * self, int allow_overscale);
void RtspPlayer__set_playback_url(RtspPlayer* self, char *url);
char * RtspPlayer__get_playback_url(RtspPlayer* self);
void RtspPlayer__set_credentials(RtspPlayer * self, char * user, char * name);
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