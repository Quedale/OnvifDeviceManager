#ifndef ONVIF_NVT_H_ 
#define ONVIF_NVT_H_

#include <glib-object.h>
#include "../gst/gstrtspplayer.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_NVT OnvifNVT__get_type()
G_DECLARE_FINAL_TYPE (OnvifNVT, OnvifNVT_, ONVIFMGR, NVT, GtkOverlay)

struct _OnvifNVT {
  GtkOverlay parent_instance;
};


struct _OnvifNVTClass {
  GtkOverlayClass parent_class;
};

GtkWidget* OnvifNVT__new(GstRtspPlayer * player);

G_END_DECLS

#endif