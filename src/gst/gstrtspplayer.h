#ifndef ONVIF_RTSPPLAYER_H_ 
#define ONVIF_RTSPPLAYER_H_

#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

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
void GstRtspPlayer__play(GstRtspPlayer* self);
void GstRtspPlayer__retry(GstRtspPlayer* self);
void GstRtspPlayer__stop(GstRtspPlayer* self);
void GstRtspPlayer__set_playback_url(GstRtspPlayer * self, char *url);
void GstRtspPlayer__set_credentials(GstRtspPlayer * self, char * user, char * pass);
GtkWidget * GstRtspPlayer__createCanvas(GstRtspPlayer *self);
gboolean GstRtspPlayer__is_mic_mute(GstRtspPlayer* self);
void GstRtspPlayer__mic_mute(GstRtspPlayer* self, gboolean mute);
void GstRtspPlayer__set_view_mode(GstRtspPlayer * self, GstRtspViewMode mode);
void GstRtspPlayer__set_port_fallback(GstRtspPlayer* self, char * port);
void GstRtspPlayer__set_host_fallback(GstRtspPlayer* self, char * host);
GstSnapshot * GstRtspPlayer__get_snapshot(GstRtspPlayer* self);

G_END_DECLS


#endif