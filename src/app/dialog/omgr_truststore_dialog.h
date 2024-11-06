#ifndef ONVIFMGR_TRUSTSTOREDIALOG_
#define ONVIFMGR_TRUSTSTOREDIALOG_

#include <gtk/gtk.h>
#include "omgr_app_dialog.h"

G_BEGIN_DECLS

#define ONVIFMGR_TYPE_TRUSTSTOREDIALOG OnvifMgrTrustStoreDialog__get_type()
G_DECLARE_FINAL_TYPE (OnvifMgrTrustStoreDialog, OnvifMgrTrustStoreDialog_, ONVIFMGR, TRUSTSTOREDIALOG, OnvifMgrAppDialog)
        
struct _OnvifMgrTrustStoreDialog
{
    OnvifMgrAppDialog parent;
};

struct _OnvifMgrTrustStoreDialogClass
{
    OnvifMgrAppDialogClass parent;
};

OnvifMgrTrustStoreDialog * OnvifMgrTrustStoreDialog__new();

G_END_DECLS

#endif