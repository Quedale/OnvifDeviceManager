#ifndef MSG_DIALOG_H_ 
#define MSG_DIALOG_H_

#include <gtk/gtk.h>
#include "app_dialog.h"

typedef struct _MsgDialog MsgDialog;

typedef struct _MsgDialog {
    AppDialog parent;
    char * msg;
    GtkWidget * icon;
    void * elements;
} MsgDialog;

MsgDialog * MsgDialog__create();
void MsgDialog__set_message(MsgDialog * self, char * msg);
void MsgDialog__set_icon(MsgDialog * self, GtkWidget * icon);

#endif