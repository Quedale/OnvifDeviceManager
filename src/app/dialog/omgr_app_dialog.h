#ifndef ONVIFMGR_APPDIALOG_H_ 
#define ONVIFMGR_APPDIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define ONVIFMGR_TYPE_APPDIALOG OnvifMgrAppDialog__get_type()
G_DECLARE_DERIVABLE_TYPE (OnvifMgrAppDialog, OnvifMgrAppDialog_, ONVIFMGR, APPDIALOG, GtkGrid)

struct _OnvifMgrAppDialogClass
{
    GtkGridClass parent;
    GtkWidget * (*create_ui)(OnvifMgrAppDialog *);
};

OnvifMgrAppDialog * OnvifMgrAppDialog__new();
void OnvifMgrAppDialog__show_loading(OnvifMgrAppDialog * self, char * message);
void OnvifMgrAppDialog__hide_loading(OnvifMgrAppDialog * self);

G_END_DECLS

#endif