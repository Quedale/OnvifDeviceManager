#ifndef ONVIF_NVTSTREAMDETAILS_H_ 
#define ONVIF_NVTSTREAMDETAILS_H_

#include <glib-object.h>
#include "../gst/gstrtspstream.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_NVTTAGPANEL OnvifNVTTagPanel__get_type()
G_DECLARE_FINAL_TYPE (OnvifNVTTagPanel, OnvifNVTTagPanel_, ONVIFMGR, NVTTAGPANEL, GtkBox)

#define ONVIFMGR_TYPE_NVTSTREAMDETAILS OnvifNVTStreamDetails__get_type()
G_DECLARE_FINAL_TYPE (OnvifNVTStreamDetails, OnvifNVTStreamDetails_, ONVIFMGR, NVTSTREAMDETAILS, GtkGrid)

struct _OnvifNVTTagPanel {
  GtkBox parent_instance;
};

struct _OnvifNVTTagPanelClass {
  GtkBoxClass parent_class;
};

struct _OnvifNVTStreamDetails {
  GtkGrid parent_instance;
};

struct _OnvifNVTStreamDetailsClass {
  GtkGridClass parent_class;
};

GtkWidget* OnvifNVTStreamDetails__new(GstRtspStream * stream);

G_END_DECLS

#endif