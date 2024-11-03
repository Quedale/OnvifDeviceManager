#ifndef ONVIFMGR_ADDDIALOG_
#define ONVIFMGR_ADDDIALOG_

#include <gtk/gtk.h>
#include "omgr_app_dialog.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_ADDDIALOG OnvifMgrAddDialog__get_type()
G_DECLARE_FINAL_TYPE (OnvifMgrAddDialog, OnvifMgrAddDialog_, ONVIFMGR, ADDDIALOG, OnvifMgrAppDialog)

struct _OnvifMgrAddDialog
{
    OnvifMgrAppDialog parent;
};

struct _OnvifMgrAddDialogClass
{
    OnvifMgrAppDialogClass parent;
};

OnvifMgrAddDialog * OnvifMgrAddDialog__new();
const char * OnvifMgrAddDialog__get_host(OnvifMgrAddDialog * self);
const char * OnvifMgrAddDialog__get_port(OnvifMgrAddDialog * self);
const char * OnvifMgrAddDialog__get_user(OnvifMgrAddDialog * self);
const char * OnvifMgrAddDialog__get_pass(OnvifMgrAddDialog * self);
const char * OnvifMgrAddDialog__get_protocol(OnvifMgrAddDialog * self);

G_END_DECLS

#endif