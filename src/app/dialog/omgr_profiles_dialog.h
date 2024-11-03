#ifndef ONVIFMGR_PROFILESDIALOG_
#define ONVIFMGR_PROFILESDIALOG_

#include <gtk/gtk.h>
#include "omgr_app_dialog.h"
#include "../../queue/event_queue.h"
#include "../omgr_device_row.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_PROFILESDIALOG OnvifMgrProfilesDialog__get_type()
G_DECLARE_FINAL_TYPE (OnvifMgrProfilesDialog, OnvifMgrProfilesDialog_, ONVIFMGR, PROFILESDIALOG, OnvifMgrAppDialog)

struct _OnvifMgrProfilesDialog
{
    OnvifMgrAppDialog parent;
};

struct _OnvifMgrProfilesDialogClass
{
    OnvifMgrAppDialogClass parent;
};

OnvifMgrProfilesDialog * OnvifMgrProfilesDialog__new(EventQueue * queue, OnvifMgrDeviceRow * device);

G_END_DECLS

#endif