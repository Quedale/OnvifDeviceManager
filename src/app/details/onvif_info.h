#ifndef ONVIF_INFO_DETAILS_H_ 
#define ONVIF_INFO_DETAILS_H_

#include "../onvif_app.h"
#include "../omgr_device_row.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_INFOPANEL OnvifInfoPanel__get_type()
G_DECLARE_FINAL_TYPE (OnvifInfoPanel, OnvifInfoPanel_, ONVIFMGR, INFOPANEL, GtkScrolledWindow)

struct _OnvifInfoPanel
{
  GtkScrolledWindow parent_instance;
};


struct _OnvifInfoPanelClass
{
  GtkScrolledWindowClass parent_class;
};

OnvifInfoPanel * OnvifInfoPanel__new(OnvifApp * app);

G_END_DECLS

#endif