#ifndef APP_DIALOG_H_ 
#define APP_DIALOG_H_

#include <gtk/gtk.h>
#include "cobject.h"

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
    int action_visible;
    int closable;
    int cancellable;
    int submittable;
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

AppDialog * AppDialog__create(GtkWidget * (*create_ui)(AppDialogEvent *));
void AppDialog__init(AppDialog* dialog, GtkWidget * (*create_ui)(AppDialogEvent *));
void AppDialog__show(AppDialog* dialog, void (*submit_callback)(AppDialogEvent *), void (*cancel_callback)(AppDialogEvent *), void * user_data);
void AppDialog__hide(AppDialog* dialog);
int AppDialog__has_focus(AppDialog* dialog);
void AppDialog__set_show_callback(AppDialog* dialog, void (*show_callback)(AppDialogEvent *));
void AppDialog__set_submit_label(AppDialog* dialog, char * label);
void AppDialog__add_to_overlay(AppDialog * self, GtkOverlay * overlay);
void AppDialog__set_destroy_callback(AppDialog* self, void (*destroy_callback)(AppDialog *)); 
void AppDialog__set_title(AppDialog * self, char * title);
void AppDialog__hide_actions(AppDialog * self);
void AppDialog__show_actions(AppDialog * self);
void AppDialog__set_closable(AppDialog * self, int closable);
void AppDialog__set_cancellable(AppDialog * self, int cancellable);
void AppDialog__set_submitable(AppDialog * self, int submitable);
void * AppDialog__get_user_data(AppDialog * self);

AppDialogEvent * AppDialogEvent_copy(AppDialogEvent * original);

#endif