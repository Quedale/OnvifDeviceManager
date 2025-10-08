#ifndef ONVIF_NETWORK_H_ 
#define ONVIF_NETWORK_H_

#include <gtk/gtk.h>
#include "onvif_details_panel.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_NETWORKPANEL OnvifNetworkPanel__get_type()
G_DECLARE_FINAL_TYPE (OnvifNetworkPanel, OnvifNetworkPanel_, ONVIFMGR, NETWORKPANEL, OnvifDetailsPanel)

struct _OnvifNetworkPanel {
  GtkScrolledWindow parent_instance;
};


struct _OnvifNetworkPanelClass {
  GtkScrolledWindowClass parent_class;
};

OnvifNetworkPanel * OnvifNetworkPanel__new(OnvifApp * app);

G_END_DECLS

#endif