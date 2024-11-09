#ifndef ONVIFMGR_APPDIALOG_H_ 
#define ONVIFMGR_APPDIALOG_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
    ONVIFMGR_APPDIALOG_BUTTON_BEFORE = 1,
    ONVIFMGR_APPDIALOG_BUTTON_MIDDLE = 3,
    ONVIFMGR_APPDIALOG_BUTTON_AFTER = 5,
} OnvifMgrAppDialogButtonPosition;


#define ONVIFMGR_TYPE_APPDIALOG OnvifMgrAppDialog__get_type()
G_DECLARE_DERIVABLE_TYPE (OnvifMgrAppDialog, OnvifMgrAppDialog_, ONVIFMGR, APPDIALOG, GtkGrid)

struct _OnvifMgrAppDialogClass
{
    GtkGridClass parent;
    GtkWidget * (*create_ui)(OnvifMgrAppDialog *);
    void (* show) (GtkWidget *widget);
};

OnvifMgrAppDialog * OnvifMgrAppDialog__new();
void OnvifMgrAppDialog__show_loading(OnvifMgrAppDialog * self, char * message);
void OnvifMgrAppDialog__hide_loading(OnvifMgrAppDialog * self);
void OnvifMgrAppDialog__add_action_widget(OnvifMgrAppDialog * self, GtkWidget * widget, OnvifMgrAppDialogButtonPosition position);

G_END_DECLS

#endif