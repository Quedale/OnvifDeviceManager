#ifndef PROFILES_DIALOG_H_ 
#define PROFILES_DIALOG_H_

typedef struct _ProfilesDialog ProfilesDialog;

#include <gtk/gtk.h>
#include "app_dialog.h"
#include "../device.h"
#include "../../queue/event_queue.h"
#include "onvif_device.h"

typedef struct _ProfilesDialog {
    AppDialog parent;
    EventQueue * queue;
    Device * device;
    void (*profile_selected)(ProfilesDialog * dialog, OnvifProfile * profile);
    void * user_data;
    void * elements;
} ProfilesDialog;

ProfilesDialog * ProfilesDialog__create(EventQueue * queue, void (* clicked)  (ProfilesDialog * dialog, OnvifProfile * profile));
void ProfilesDialog__set_device(ProfilesDialog * self, Device * device);

#endif