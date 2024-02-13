#ifndef ONVIF_INFO_DETAILS_H_ 
#define ONVIF_INFO_DETAILS_H_

#include "onvif_app.h"
#include "omgr_device_row.h"

typedef struct _OnvifInfo OnvifInfo;

OnvifInfo * OnvifInfo__create(OnvifApp * app);
void OnvifInfo__destroy(OnvifInfo* self);
void OnvifInfo_update_details(OnvifInfo * self, OnvifMgrDeviceRow * device);
void OnvifInfo_clear_details(OnvifInfo * self);
GtkWidget * OnvifInfo__get_widget(OnvifInfo * self);
void OnvifInfo__set_done_callback(OnvifInfo* self, void (*done_callback)(OnvifInfo *, void *), void * user_data);

#endif