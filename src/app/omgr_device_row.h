#ifndef NEW_DEVICE_LIST_H_ 
#define NEW_DEVICE_LIST_H_

typedef struct _OnvifMgrDeviceRow OnvifMgrDeviceRow;

#include <gtk/gtk.h>
#include "onvif_device.h"
#include "onvif_app.h"
#include "c_ownable_interface.h"

G_BEGIN_DECLS

#define ONVIFMGR_DEVICEROW_TRAIL(fmt,dev, ...) ONVIF_DEVICE_TRAIL(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)
#define ONVIFMGR_DEVICEROW_TRACE(fmt,dev, ...) ONVIF_DEVICE_TRACE(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)
#define ONVIFMGR_DEVICEROW_DEBUG(fmt,dev, ...) ONVIF_DEVICE_DEBUG(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)
#define ONVIFMGR_DEVICEROW_INFO(fmt,dev, ...) ONVIF_DEVICE_INFO(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)
#define ONVIFMGR_DEVICEROW_WARN(fmt,dev, ...) ONVIF_DEVICE_WARN(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)
#define ONVIFMGR_DEVICEROW_ERROR(fmt,dev, ...) ONVIF_DEVICE_ERROR(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)
#define ONVIFMGR_DEVICEROW_FATAL(fmt,dev, ...) ONVIF_DEVICE_FATAL(fmt, OnvifMgrDeviceRow__get_device(ONVIFMGR_DEVICEROW(dev)),##__VA_ARGS__)

#define ONVIFMGR_TYPE_DEVICEROW OnvifMgrDeviceRow__get_type()
G_DECLARE_FINAL_TYPE (OnvifMgrDeviceRow, OnvifMgrDeviceRow_, ONVIFMGR, DEVICEROW, GtkGrid)

#define ONVIFMGR_IS_DEVICEROWROW_VALID(x) (ONVIFMGR_IS_DEVICEROW(x) && COwnableObject__has_owner(COWNABLE_OBJECT(x)))


struct _OnvifMgrDeviceRow
{
  GtkListBoxRow parent_instance;
};


struct _OnvifMgrDeviceRowClass
{
  GtkListBoxRowClass parent_class;
};

GtkWidget* OnvifMgrDeviceRow__new(OnvifApp * app, OnvifDevice * device, char * name, char * hardware, char * location);
OnvifApp * OnvifMgrDeviceRow__get_app(OnvifMgrDeviceRow * self);
void OnvifMgrDeviceRow__set_device(OnvifMgrDeviceRow * self, OnvifDevice * device);
OnvifDevice * OnvifMgrDeviceRow__get_device(OnvifMgrDeviceRow * self);
void OnvifMgrDeviceRow__set_name(OnvifMgrDeviceRow * self, char * name);
void OnvifMgrDeviceRow__set_hardware(OnvifMgrDeviceRow * self, char * hardware);
void OnvifMgrDeviceRow__set_location(OnvifMgrDeviceRow * self, char * location);
void OnvifMgrDeviceRow__set_profile(OnvifMgrDeviceRow * self, OnvifProfile * profile);
OnvifProfile * OnvifMgrDeviceRow__get_profile(OnvifMgrDeviceRow * self);
gboolean OnvifMgrDeviceRow__is_selected(OnvifMgrDeviceRow * self);

void OnvifMgrDeviceRow__load_thumbnail(OnvifMgrDeviceRow * self);
void OnvifMgrDeviceRow__set_thumbnail(OnvifMgrDeviceRow * self, GtkWidget * image);

G_END_DECLS

#endif