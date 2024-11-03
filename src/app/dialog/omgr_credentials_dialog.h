#ifndef ONVIFMGR_CREDENTIALSDIALOG_H_ 
#define ONVIFMGR_CREDENTIALSDIALOG_H_

#include <gtk/gtk.h>
#include "omgr_app_dialog.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_CREDENTIALSDIALOG OnvifMgrCredentialsDialog__get_type()
G_DECLARE_FINAL_TYPE (OnvifMgrCredentialsDialog, OnvifMgrCredentialsDialog_, ONVIFMGR, CREDENTIALSDIALOG, OnvifMgrAppDialog)

struct _OnvifMgrCredentialsDialog
{
    OnvifMgrAppDialog parent;
};

struct _OnvifMgrCredentialsDialogClass
{
    OnvifMgrAppDialogClass parent;
};

OnvifMgrCredentialsDialog * OnvifMgrCredentialsDialog__new();
const char * OnvifMgrCredentialsDialog__get_username(OnvifMgrCredentialsDialog * self);
const char * OnvifMgrCredentialsDialog__get_password(OnvifMgrCredentialsDialog * self);

G_END_DECLS

#endif