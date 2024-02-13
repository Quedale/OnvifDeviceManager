#ifndef PROFILES_DIALOG_H_ 
#define PROFILES_DIALOG_H_

typedef struct _ProfilesDialog ProfilesDialog;

#include <gtk/gtk.h>
#include "app_dialog.h"
#include "../../queue/event_queue.h"
#include "onvif_device.h"
#include "../omgr_device_row.h"

typedef struct _ProfilesDialog {
    AppDialog parent;
    EventQueue * queue;
    OnvifMgrDeviceRow * device;
    void (*profile_selected)(ProfilesDialog * dialog, OnvifProfile * profile);
    void * user_data;
    void * elements;
} ProfilesDialog;

ProfilesDialog * ProfilesDialog__create(EventQueue * queue, void (* clicked)  (ProfilesDialog * dialog, OnvifProfile * profile));
void ProfilesDialog__set_device(ProfilesDialog * self, OnvifMgrDeviceRow * device);
OnvifMgrDeviceRow * ProfilesDialog__get_device(ProfilesDialog * self);

#endif