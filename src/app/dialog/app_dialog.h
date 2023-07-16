#ifndef APP_DIALOG_H_ 
#define APP_DIALOG_H_

#include <gtk/gtk.h>
#include "../../oo/cobject.h"

#define APPDIALOG_TITLE_PREFIX "<span size=\"x-large\">"
#define APPDIALOG_TITLE_SUFFIX "</span>"

typedef enum AppDialogEventType {
    APP_DIALOG_SUBMIT_EVENT = 0,
    APP_DIALOG_CANCEL_EVENT = 1,
    APP_DIALOG_SHOW_EVENT = 2,
    APP_DIALOG_UI_EVENT = 3,
} AppDialogEventType;

typedef struct _AppDialog AppDialog;
typedef struct _AppDialogEvent AppDialogEvent;

struct _AppDialog {
    CObject parent;
    int visible;
    char * title;
    char * submit_label;
    GtkWidget * root;
    void (*submit_callback)(AppDialogEvent *);
    void (*cancel_callback)(AppDialogEvent *);
    void (*show_callback)(AppDialogEvent *);
    GtkWidget * (*create_ui)(AppDialogEvent *);
    void (*destroy_callback)(AppDialog *);
    void * user_data;
    void * private;
};

struct _AppDialogEvent { 
    AppDialogEventType type;
    AppDialog * dialog;
    void * user_data;
};

AppDialog * AppDialog__create(char * title, char * submit_label, GtkWidget * (*create_ui)(AppDialogEvent *));
void AppDialog__init(AppDialog* dialog, char * title, char * submit_label, GtkWidget * (*create_ui)(AppDialogEvent *));
void AppDialog__show(AppDialog* dialog, void (*submit_callback)(AppDialogEvent *), void (*cancel_callback)(AppDialogEvent *), void * user_data);
void AppDialog__hide(AppDialog* dialog);
void AppDialog__set_show_callback(AppDialog* dialog, void (*show_callback)(AppDialogEvent *));
void AppDialog__set_submit_label(char * label);
void AppDialog__add_to_overlay(AppDialog * self, GtkOverlay * overlay);
void AppDialog__set_destroy_callback(AppDialog* self, void (*destroy_callback)(AppDialog *)); 

AppDialogEvent * AppDialogEvent_copy(AppDialogEvent * original);

#endif