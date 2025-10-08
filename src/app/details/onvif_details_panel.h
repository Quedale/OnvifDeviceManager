#ifndef ONVIF_DETAILS_PANEL_H_ 
#define ONVIF_DETAILS_PANEL_H_

#include "../onvif_app.h"
#include "../omgr_device_row.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_DETAILSPANEL OnvifDetailsPanel__get_type()
G_DECLARE_DERIVABLE_TYPE (OnvifDetailsPanel, OnvifDetailsPanel_, ONVIFMGR, DETAILSPANEL, GtkScrolledWindow)

struct _OnvifDetailsPanelClass {
  GtkScrolledWindowClass parent_class;
  void * (* getdata)  (OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, QueueEvent * qevt);
  void (* updateui)  (OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, void * user_data);
  void (* clearui)  (OnvifDetailsPanel * self);
  void (* createui)  (OnvifDetailsPanel * self);
};

G_END_DECLS

#endif