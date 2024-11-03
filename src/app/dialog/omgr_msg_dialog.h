#ifndef ONVIFMGR_MSGDIALOG_
#define ONVIFMGR_MSGDIALOG_

#include <gtk/gtk.h>
#include "omgr_app_dialog.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_MSGDIALOG OnvifMgrMsgDialog__get_type()
G_DECLARE_FINAL_TYPE (OnvifMgrMsgDialog, OnvifMgrMsgDialog_, ONVIFMGR, MSGDIALOG, OnvifMgrAppDialog)

struct _OnvifMgrMsgDialog
{
    OnvifMgrAppDialog parent;
};

struct _OnvifMgrMsgDialogClass
{
    OnvifMgrAppDialogClass parent;
};

OnvifMgrMsgDialog * OnvifMgrMsgDialog__new();

G_END_DECLS

#endif