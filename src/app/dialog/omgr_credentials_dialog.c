#include "omgr_credentials_dialog.h"
#include "clogger.h"

#define CREDENTIALS_DIALOG_TITLE "ONVIF Device Authentication"
#define CREDENTIALS_DIALOG_LABEL "Login"

typedef struct {
    GtkWidget * txtuser;
    GtkWidget * txtpassword;
} OnvifMgrCredentialsDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrCredentialsDialog, OnvifMgrCredentialsDialog_, ONVIFMGR_TYPE_APPDIALOG)

const char * 
OnvifMgrCredentialsDialog__get_username(OnvifMgrCredentialsDialog * self){
    g_return_val_if_fail (self != NULL,NULL);
    g_return_val_if_fail (ONVIFMGR_IS_CREDENTIALSDIALOG (self),NULL);
    OnvifMgrCredentialsDialogPrivate *priv = OnvifMgrCredentialsDialog__get_instance_private (self);
    return gtk_entry_get_text(GTK_ENTRY(priv->txtuser));
}

const char * 
OnvifMgrCredentialsDialog__get_password(OnvifMgrCredentialsDialog * self){
    g_return_val_if_fail (self != NULL,NULL);
    g_return_val_if_fail (ONVIFMGR_IS_CREDENTIALSDIALOG (self),NULL);
    OnvifMgrCredentialsDialogPrivate *priv = OnvifMgrCredentialsDialog__get_instance_private (self);
    return gtk_entry_get_text(GTK_ENTRY(priv->txtpassword));
}

static GtkWidget * 
OnvifMgrCredentialsDialog__create_ui(OnvifMgrAppDialog * app_dialog){
    OnvifMgrCredentialsDialogPrivate *priv = OnvifMgrCredentialsDialog__get_instance_private (ONVIFMGR_CREDENTIALSDIALOG(app_dialog));
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();
    
    widget = gtk_label_new("Username :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

    priv->txtuser = gtk_entry_new();
    g_object_set (priv->txtuser, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (priv->txtuser, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txtuser, 1, 0, 1, 1);
    
    widget = gtk_label_new("Password :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

    priv->txtpassword = gtk_entry_new();
    g_object_set (priv->txtpassword, "margin-right", 10, NULL);
    gtk_entry_set_visibility(GTK_ENTRY(priv->txtpassword),FALSE);
    gtk_widget_set_hexpand (priv->txtpassword, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txtpassword, 1, 1, 1, 1);

    return grid;
}

static void
OnvifMgrCredentialsDialog__show (GtkWidget *widget){
    OnvifMgrCredentialsDialogPrivate *priv = OnvifMgrCredentialsDialog__get_instance_private (ONVIFMGR_CREDENTIALSDIALOG(widget));

    //Steal focus
    gtk_widget_grab_focus(priv->txtuser);
    
    //Clear previous credentials
    gtk_entry_set_text(GTK_ENTRY(priv->txtuser),"");
    gtk_entry_set_text(GTK_ENTRY(priv->txtpassword),"");

    GTK_WIDGET_CLASS (OnvifMgrCredentialsDialog__parent_class)->show (widget);
}

static void
OnvifMgrCredentialsDialog__class_init (OnvifMgrCredentialsDialogClass *klass){
    OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
    GtkWidgetClass * widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->show = OnvifMgrCredentialsDialog__show;
    app_class->create_ui = OnvifMgrCredentialsDialog__create_ui;
}

static void
OnvifMgrCredentialsDialog__init (OnvifMgrCredentialsDialog *self){

    g_object_set (self, 
            "title-label", CREDENTIALS_DIALOG_TITLE,
            "submit-label", CREDENTIALS_DIALOG_LABEL,
            NULL);
}

OnvifMgrCredentialsDialog * 
OnvifMgrCredentialsDialog__new(){
    return g_object_new (ONVIFMGR_TYPE_CREDENTIALSDIALOG,
                        NULL);
}