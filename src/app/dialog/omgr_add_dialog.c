#include "omgr_add_dialog.h"
#include "clogger.h"
#include <ctype.h>

#define ONVIFMGR_ADDDIALOG_TITLE "ONVIF Add Device"
#define ONVIFMGR_ADDDIALOG_SUBMIT_LABEL "Add"

const char * ONVIFMGR_ADDDIALOG_HTTP_CONST_STRING = "http";
const char * ONVIFMGR_ADDDIALOG_HTTPS_CONST_STRING = "https";

typedef struct {
    GtkWidget * txthost;
    GtkWidget * txtport;
    GtkWidget * txtuser;
    GtkWidget * txtpass;
    GtkWidget * chkhttps;
} OnvifMgrAddDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrAddDialog, OnvifMgrAddDialog_, ONVIFMGR_TYPE_APPDIALOG)

const char * 
OnvifMgrAddDialog__get_host(OnvifMgrAddDialog * self){
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (self);
    return gtk_entry_get_text(GTK_ENTRY(priv->txthost));
}

const char * 
OnvifMgrAddDialog__get_port(OnvifMgrAddDialog * self){
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (self);
    const char * port = gtk_entry_get_text(GTK_ENTRY(priv->txtport));
    if(!port || strlen(port) == 0){
        port = gtk_entry_get_placeholder_text(GTK_ENTRY(priv->txtport));
    }
    return port;
}

const char * 
OnvifMgrAddDialog__get_protocol(OnvifMgrAddDialog * self){
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (self);
    if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->chkhttps))){
        return ONVIFMGR_ADDDIALOG_HTTPS_CONST_STRING;
    } else {
        return ONVIFMGR_ADDDIALOG_HTTP_CONST_STRING;
    }
}

const char * 
OnvifMgrAddDialog__get_user(OnvifMgrAddDialog * self){
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (self);
    return gtk_entry_get_text(GTK_ENTRY(priv->txtuser));
}

const char * 
OnvifMgrAddDialog__get_pass(OnvifMgrAddDialog * self){
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (self);
    return gtk_entry_get_text(GTK_ENTRY(priv->txtpass));
}

static void 
OnvifMgrAddDialog__port_text_validate(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data){
    int i;
    //Check if new characters are numbers
    for (i = 0; i < length; i++) {
        if (!isdigit(text[i])) {
            g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
            return;
        }
    }

    //Check if the result is a valid port range
    const char * oldtext = gtk_entry_get_text(GTK_ENTRY(editable));
    char ntext[strlen(oldtext) + 2];
    strcpy(ntext,oldtext);
    strcat(ntext,text);
    i = atoi (ntext);
    if(i < 0 || i > 65535){
        g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
    }
}

static int 
OnvifMgrAddDialog__check_for_valid_host_char(const char c){
	return isalnum(c) || c == '.' || c == '-' || c == '_';
}

static void 
OnvifMgrAddDialog__host_text_validate(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data){
    const char * oldtext = gtk_entry_get_text(GTK_ENTRY(editable));
    char ntext[strlen(oldtext) + strlen(text) + 1];
    strcpy(ntext,oldtext);
    strcat(ntext,text);

	for (const char *p = ntext; *p; p++){
		if (!OnvifMgrAddDialog__check_for_valid_host_char(*p)) {
            g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
            return;
        }
	}
}

static void 
OnvifMgrAddDialog__protocol_check_cb (GtkToggleButton *source, GtkWidget * txtport) {
    if(gtk_toggle_button_get_active (source)){
        gtk_entry_set_placeholder_text(GTK_ENTRY(txtport),"443");
    } else {
        gtk_entry_set_placeholder_text(GTK_ENTRY(txtport),"80");
    }
}

static GtkWidget * 
OnvifMgrAddDialog__create_ui(OnvifMgrAppDialog * app_dialog){
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (ONVIFMGR_ADDDIALOG(app_dialog));
    GtkWidget * widget;
    GtkWidget * grid;

    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing(GTK_GRID(grid),10);
    gtk_grid_set_row_spacing(GTK_GRID(grid),5);

    widget = gtk_label_new("Hostname/IP :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);
    
    priv->txthost = gtk_entry_new();
    gtk_widget_set_margin_end(priv->txthost,10);
    gtk_widget_set_hexpand (priv->txthost, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txthost, 1, 1, 1, 1);
    g_signal_connect(G_OBJECT(priv->txthost), "insert-text", G_CALLBACK(OnvifMgrAddDialog__host_text_validate), NULL);

    widget = gtk_label_new("Port :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 3, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    priv->txtport = gtk_entry_new();
    gtk_widget_set_margin_end(priv->txtport,10);
    gtk_entry_set_width_chars(GTK_ENTRY(priv->txtport),5);
    gtk_widget_set_hexpand (priv->txtport, FALSE);
    g_signal_connect(G_OBJECT(priv->txtport), "insert-text", G_CALLBACK(OnvifMgrAddDialog__port_text_validate), NULL);
    gtk_entry_set_placeholder_text(GTK_ENTRY(priv->txtport),"80");

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), priv->txtport, FALSE, FALSE, 0);
    gtk_grid_attach (GTK_GRID (grid), vbox, 1, 3, 1, 1);
    
    widget = gtk_label_new("HTTPS :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    priv->chkhttps = gtk_check_button_new();
    gtk_widget_set_margin_end(priv->chkhttps,10);
    gtk_widget_set_hexpand (priv->chkhttps, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->chkhttps, 1, 2, 1, 1);
    g_signal_connect (priv->chkhttps, "toggled",
                    G_CALLBACK (OnvifMgrAddDialog__protocol_check_cb),
                    priv->txtport);

    widget = gtk_label_new("User :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 4, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    priv->txtuser = gtk_entry_new();
    gtk_widget_set_margin_end(priv->txtuser,10);
    gtk_widget_set_hexpand (priv->txtuser, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txtuser, 1, 4, 1, 1);

    widget = gtk_label_new("Password :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 5, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    priv->txtpass = gtk_entry_new();
    gtk_widget_set_margin_end(priv->txtpass,10);
    gtk_entry_set_visibility(GTK_ENTRY(priv->txtpass),FALSE);
    gtk_widget_set_hexpand (priv->txtpass, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txtpass, 1, 5, 1, 1);

    g_object_set (app_dialog, 
            "title-label", ONVIFMGR_ADDDIALOG_TITLE,
            "submit-label", ONVIFMGR_ADDDIALOG_SUBMIT_LABEL,
            NULL);

    return grid;
}

static void
OnvifMgrAddDialog__show (GtkWidget *widget){
    OnvifMgrAddDialog * self = ONVIFMGR_ADDDIALOG(widget);
    OnvifMgrAddDialogPrivate *priv = OnvifMgrAddDialog__get_instance_private (self);

    //Steal focus
    gtk_widget_grab_focus(priv->txthost);
    //Clear previous device uri
    gtk_entry_set_text(GTK_ENTRY(priv->txthost),"");
    gtk_entry_set_text(GTK_ENTRY(priv->txtport),"");
    gtk_entry_set_text(GTK_ENTRY(priv->txtuser),"");
    gtk_entry_set_text(GTK_ENTRY(priv->txtpass),"");
    g_object_set(self,"error",NULL,NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->chkhttps),FALSE);

    GTK_WIDGET_CLASS (OnvifMgrAddDialog__parent_class)->show (widget);
}

static void
OnvifMgrAddDialog__class_init (OnvifMgrAddDialogClass *klass){
    OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
    app_class->create_ui = OnvifMgrAddDialog__create_ui;
    GtkWidgetClass * widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->show = OnvifMgrAddDialog__show;
}

static void
OnvifMgrAddDialog__init (OnvifMgrAddDialog *self){

}

OnvifMgrAddDialog * 
OnvifMgrAddDialog__new(){
    return g_object_new (ONVIFMGR_TYPE_ADDDIALOG,
                        NULL);
}