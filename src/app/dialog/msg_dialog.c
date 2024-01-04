#include "msg_dialog.h"
#include "../../queue/event_queue.h"
#include "clogger.h"
#include "../gui_utils.h"
#include "app_dialog.h"

#define MSG_DIALOG_SUBMIT_LABEL "Ok"

typedef struct { 
    GtkWidget * lblmessage;
    GtkWidget * lbltitle;
    GtkWidget * image_handle;
} DialogElements;

GtkWidget * priv_MsgDialog__create_ui(AppDialogEvent * event);
void priv_MsgDialog__destroy(AppDialog * dialog);
void priv_MsgDialog__update_msg(MsgDialog * dialog);

GtkWidget * priv_MsgDialog__create_ui(AppDialogEvent * event){
    MsgDialog * dialog = (MsgDialog *) event->dialog; 
    GtkWidget * grid = gtk_grid_new ();
    DialogElements * elements = (DialogElements *) dialog->elements;

    elements->lblmessage = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(elements->lblmessage),1);
    gtk_widget_set_hexpand (elements->lblmessage, TRUE);
    gtk_widget_set_vexpand (elements->lblmessage, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->lblmessage, 0, 1, 3, 1);
    gtk_label_set_justify(GTK_LABEL(elements->lblmessage), GTK_JUSTIFY_CENTER);

    GtkWidget * empty = gtk_label_new("");
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (grid), empty, 0, 0, 1, 1);

    GtkWidget * thumbnail_handle = gtk_event_box_new ();
    elements->image_handle = thumbnail_handle;
    if(dialog->icon){
        gtk_container_add (GTK_CONTAINER (thumbnail_handle), dialog->icon);
    }
    g_object_set (thumbnail_handle, "margin-top", 20, NULL);
    gtk_grid_attach (GTK_GRID (grid), thumbnail_handle, 1, 0, 1, 1);

    empty = gtk_label_new("");
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (grid), empty, 2, 0, 1, 1);

    return grid;
}

void MsgDialog__reset(MsgDialog* self) {
    MsgDialog__set_message(self,NULL);
    MsgDialog__set_icon(self,NULL);
    AppDialog__set_closable((AppDialog*)self,1);
    AppDialog__show_actions((AppDialog*)self);
    AppDialog__set_title((AppDialog*)self,NULL);
    AppDialog__set_submit_label((AppDialog*)self,MSG_DIALOG_SUBMIT_LABEL);
}

MsgDialog * MsgDialog__create(){
    C_TRACE("create");
    MsgDialog * dialog = malloc(sizeof(MsgDialog));
    dialog->elements = malloc(sizeof(DialogElements));
    dialog->icon = NULL;
    dialog->msg = NULL;

    AppDialog__init((AppDialog *)dialog, priv_MsgDialog__create_ui);
    CObject__set_allocated((CObject *) dialog);
    AppDialog__set_destroy_callback((AppDialog*)dialog,priv_MsgDialog__destroy);

    MsgDialog__reset(dialog);
    
    return dialog;
}

void priv_MsgDialog__destroy(AppDialog * dialog){
    MsgDialog* cdialog = (MsgDialog*)dialog;
    if(cdialog->msg){
        free(cdialog->msg);
    }
    free(cdialog->elements);
}

void MsgDialog__set_message(MsgDialog * self, char * msg){
    if(!msg){
        if(self->msg){
            free(self->msg);
        }
        return;
    }
    if(!self->msg){
        self->msg = malloc(strlen(msg)+1);
    } else {
        self->msg = realloc(self->msg,strlen(msg)+1);
    }
    strcpy(self->msg,msg);
    priv_MsgDialog__update_msg(self);
}

void MsgDialog__set_icon(MsgDialog * self, GtkWidget * icon){
    self->icon = icon;

    DialogElements * elements = (DialogElements *) self->elements;
    if(elements && GTK_IS_WIDGET(elements->image_handle)){
        gui_update_widget_image(self->icon,elements->image_handle);
    }
}

void priv_MsgDialog__update_msg(MsgDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    if(elements && GTK_IS_WIDGET(elements->lblmessage)){
        gtk_label_set_text (GTK_LABEL (elements->lblmessage), dialog->msg);
    }
}