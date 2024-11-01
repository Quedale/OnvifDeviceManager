#ifndef ONVIF_RTSPPLAYER_H_ 
#define ONVIF_RTSPPLAYER_H_

#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstRtspPlayerSession GstRtspPlayerSession;
typedef struct _GstRtspPlayer GstRtspPlayer;

#define GST_TYPE_RTSPPLAYER GstRtspPlayer__get_type()
G_DECLARE_FINAL_TYPE (GstRtspPlayer, GstRtspPlayer_, GST, RTSPPLAYER, GObject)

typedef enum {
  GST_RTSP_PLAYER_VIEW_MODE_FIT_WINDOW,
  GST_RTSP_PLAYER_VIEW_MODE_FILL_WINDOW,
  GST_RTSP_PLAYER_VIEW_MODE_NATIVE
} GstRtspViewMode;

typedef struct {
  guint8* data;
  gsize size;
} GstSnapshot;

void GstSnapshot__destroy(GstSnapshot * snapshot);

struct _GstRtspPlayer
{
  GObject parent_instance;
};


struct _GstRtspPlayerClass
{
  GObjectClass parent_class;
};

GstRtspPlayer * GstRtspPlayer__new ();
void GstRtspPlayer__play(GstRtspPlayer* self, char *url, char * user, char * pass, char * fallback_host, char * fallback_port, void * user_data);
void GstRtspPlayer__stop(GstRtspPlayer* self);
GtkWidget * GstRtspPlayer__createCanvas(GstRtspPlayer *self);
gboolean GstRtspPlayer__is_mic_mute(GstRtspPlayer* self);
void GstRtspPlayer__mic_mute(GstRtspPlayer* self, gboolean mute);
void GstRtspPlayer__set_view_mode(GstRtspPlayer * self, GstRtspViewMode mode);
GstSnapshot * GstRtspPlayer__get_snapshot(GstRtspPlayer* self);
GstRtspPlayerSession * GstRtspPlayer__get_session (GstRtspPlayer * self);

void GstRtspPlayerSession__retry(GstRtspPlayerSession* state);
void * GstRtspPlayerSession__get_user_data(GstRtspPlayerSession * state);
char * GstRtspPlayerSession__get_uri(GstRtspPlayerSession * state);

G_END_DECLS


#endif