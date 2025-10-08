#ifndef ONVIF_TIMEDATE_H_ 
#define ONVIF_TIMEDATE_H_

#include <gtk/gtk.h>
#include "../onvif_app.h"
#include "../omgr_device_row.h"
#include "onvif_details_panel.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_TIMEDATEPANEL OnvifTimeDatePanel__get_type()
G_DECLARE_FINAL_TYPE (OnvifTimeDatePanel, OnvifTimeDatePanel_, ONVIFMGR, TIMEDATEPANEL, OnvifDetailsPanel)

struct _OnvifTimeDatePanel {
  GtkScrolledWindow parent_instance;
};


struct _OnvifTimeDatePanelClass {
  GtkScrolledWindowClass parent_class;
};

OnvifTimeDatePanel * OnvifTimeDatePanel__new(OnvifApp * app);

G_END_DECLS

#endif