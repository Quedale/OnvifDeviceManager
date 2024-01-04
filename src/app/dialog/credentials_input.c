#include "credentials_input.h"
#include "../../queue/event_queue.h"
#include "clogger.h"

#define CREDENTIALS_DIALOG_TITLE "ONVIF Device Authentication"
#define CREDENTIALS_DIALOG_LABEL "Login"

typedef struct { 
    GtkWidget * txtuser;
    GtkWidget * txtpassword;
} DialogElements;

GtkWidget * priv_CredentialsDialog__create_ui(AppDialogEvent * event);
void priv_CredentialsDialog__show_cb(AppDialogEvent * event);
void priv_CredentialsDialog__destroy(AppDialog* dialog);

const char * CredentialsDialog__get_username(CredentialsDialog * self){
    DialogElements * elements = (DialogElements *) self->elements;
    return gtk_entry_get_text(GTK_ENTRY(elements->txtuser));
}

const char * CredentialsDialog__get_password(CredentialsDialog * self){
    DialogElements * elements = (DialogElements *) self->elements;
    return gtk_entry_get_text(GTK_ENTRY(elements->txtpassword));
}

CredentialsDialog * CredentialsDialog__create(){
    C_TRACE("create");
    CredentialsDialog * dialog = malloc(sizeof(CredentialsDialog));
    dialog->elements = malloc(sizeof(DialogElements));
    AppDialog__init((AppDialog*)dialog,priv_CredentialsDialog__create_ui);
    CObject__set_allocated((CObject *) dialog);
    AppDialog__set_title((AppDialog *)dialog,CREDENTIALS_DIALOG_TITLE);
    AppDialog__set_submit_label((AppDialog *)dialog,CREDENTIALS_DIALOG_LABEL);
    AppDialog__set_show_callback((AppDialog *)dialog,priv_CredentialsDialog__show_cb);
    AppDialog__set_destroy_callback((AppDialog*)dialog,priv_CredentialsDialog__destroy);
    return dialog;
}

void priv_CredentialsDialog__destroy(AppDialog * dialog){
    CredentialsDialog* cdialog = (CredentialsDialog*)dialog;
    free(cdialog->elements);
}

GtkWidget * priv_CredentialsDialog__create_ui(AppDialogEvent * event){
    CredentialsDialog * dialog = (CredentialsDialog *) event->dialog;
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();
    DialogElements * elements = (DialogElements *) dialog->elements;
    
    widget = gtk_label_new("Username :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

    elements->txtuser = gtk_entry_new();
    g_object_set (elements->txtuser, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->txtuser, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtuser, 1, 0, 1, 1);
    
    widget = gtk_label_new("Password :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

    elements->txtpassword = gtk_entry_new();
    g_object_set (elements->txtpassword, "margin-right", 10, NULL);
    gtk_entry_set_visibility(GTK_ENTRY(elements->txtpassword),FALSE);
    gtk_widget_set_hexpand (elements->txtpassword, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtpassword, 1, 1, 1, 1);

    return grid;
}

void priv_CredentialsDialog__show_cb(AppDialogEvent * event){
    CredentialsDialog * dialog = (CredentialsDialog *) event->dialog;
    DialogElements * elements = (DialogElements *) dialog->elements;

    //Steal focus
    gtk_widget_grab_focus(elements->txtuser);
    
    //Clear previous credentials
    gtk_entry_set_text(GTK_ENTRY(elements->txtuser),"");
    gtk_entry_set_text(GTK_ENTRY(elements->txtpassword),"");
}