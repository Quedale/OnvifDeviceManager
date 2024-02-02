#include "add_device.h"
#include "../../queue/event_queue.h"
#include "app_dialog.h"
#include <ctype.h>

#define ADD_DEVICE_TITLE "ONVIF Add Device"
#define ADD_DEVICE_SUBMIT_LABEL "Add"


const char * HTTP_CONST_STRING = "http";
const char * HTTPS_CONST_STRING = "https";

typedef struct { 
    GtkWidget * txthost;
    GtkWidget * txtport;
    GtkWidget * txtuser;
    GtkWidget * txtpass;
    GtkWidget * lblerr;
    GtkWidget * chkhttps;
} DialogElements;

typedef struct {
    AddDeviceDialog * dialog;
    char * error;
} AddDeviceDialogErrorData;

GtkWidget * priv_AddDeviceDialog__create_iu(AppDialogEvent * event);
void priv_AddDeviceDialog__show_cb(AppDialogEvent *);
void priv_AddDeviceDialog__destroy(AppDialog * dialog);

void port_text_validate(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data){
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

int check_for_valid_host_char(const char c){
	return isalnum(c) || c == '.' || c == '-' || c == '_';
}

void host_text_validate(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data){
    const char * oldtext = gtk_entry_get_text(GTK_ENTRY(editable));
    char ntext[strlen(oldtext) + 2];
    strcpy(ntext,oldtext);
    strcat(ntext,text);

	for (const char *p = ntext; *p; p++){
		if (!check_for_valid_host_char(*p)) {
            g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
            return;
        }
	}
}

static void protocol_check_cb (GtkToggleButton *source, GtkWidget * txtport) {
    if(gtk_toggle_button_get_active (source)){
        gtk_entry_set_placeholder_text(GTK_ENTRY(txtport),"443");
    } else {
        gtk_entry_set_placeholder_text(GTK_ENTRY(txtport),"80");
    }
}

gboolean * gui_AddDeviceDialog__set_error(void * user_data){
    AddDeviceDialogErrorData * data = (AddDeviceDialogErrorData *) user_data;
    DialogElements * elements = (DialogElements *) data->dialog->elements;

    gtk_widget_set_visible(elements->lblerr,TRUE);
    gtk_label_set_label(GTK_LABEL(elements->lblerr),data->error);
    free(data->error);
    free(data);
    return FALSE;
}

void AddDeviceDialog__set_error(AddDeviceDialog * self, char * error){
    AddDeviceDialogErrorData * data = malloc(sizeof(AddDeviceDialogErrorData));
    data->dialog = self;
    data->error = malloc(strlen(error)+1);
    strcpy(data->error,error);
    gdk_threads_add_idle((void *)gui_AddDeviceDialog__set_error,data);
}

GtkWidget * priv_AddDeviceDialog__create_iu(AppDialogEvent * event){
    GtkWidget * widget;
    GtkWidget * grid;
    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    AddDeviceDialog * dialog = (AddDeviceDialog *) event->dialog; 
    DialogElements * elements = (DialogElements *) dialog->elements;
    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing(GTK_GRID(grid),10);
    gtk_grid_set_row_spacing(GTK_GRID(grid),5);
    
    elements->lblerr = gtk_label_new("");
    gtk_widget_set_hexpand (elements->lblerr, TRUE);
    gtk_widget_set_vexpand (elements->lblerr, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->lblerr, 0, 0, 2, 1);
    gtk_label_set_justify(GTK_LABEL(elements->lblerr), GTK_JUSTIFY_CENTER);

    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; color:#DE5E09;}",-1,NULL); 
    context = gtk_widget_get_style_context(elements->lblerr);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssProvider);

    widget = gtk_label_new("Hostname/IP :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);
    
    elements->txthost = gtk_entry_new();
    g_object_set (elements->txthost, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->txthost, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txthost, 1, 1, 1, 1);
    g_signal_connect(G_OBJECT(elements->txthost), "insert-text", G_CALLBACK(host_text_validate), NULL);

    widget = gtk_label_new("Port :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 3, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    elements->txtport = gtk_entry_new();
    g_object_set (elements->txtport, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->txtport, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtport, 1, 3, 1, 1);
    g_signal_connect(G_OBJECT(elements->txtport), "insert-text", G_CALLBACK(port_text_validate), NULL);
    gtk_entry_set_placeholder_text(GTK_ENTRY(elements->txtport),"80");

    widget = gtk_label_new("HTTPS :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    elements->chkhttps = gtk_check_button_new();
    g_object_set (elements->chkhttps, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->chkhttps, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->chkhttps, 1, 2, 1, 1);
    g_signal_connect (elements->chkhttps, "toggled",
                    G_CALLBACK (protocol_check_cb),
                    elements->txtport);

    widget = gtk_label_new("User :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 4, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    elements->txtuser = gtk_entry_new();
    g_object_set (elements->txtuser, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->txtuser, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtuser, 1, 4, 1, 1);

    widget = gtk_label_new("Password :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 5, 1, 1);
    gtk_widget_set_halign (widget, GTK_ALIGN_END);

    elements->txtpass = gtk_entry_new();
    g_object_set (elements->txtpass, "margin-right", 10, NULL);
    gtk_entry_set_visibility(GTK_ENTRY(elements->txtpass),FALSE);
    gtk_widget_set_hexpand (elements->txtpass, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtpass, 1, 5, 1, 1);

    return grid;
}

AddDeviceDialog * AddDeviceDialog__create(){
    AddDeviceDialog * dialog = malloc(sizeof(AddDeviceDialog));
    DialogElements * elements = malloc(sizeof(DialogElements));
    dialog->elements = elements;

    AppDialog__init((AppDialog *)dialog, priv_AddDeviceDialog__create_iu);
    CObject__set_allocated((CObject *) dialog);
    AppDialog__set_title((AppDialog *)dialog,ADD_DEVICE_TITLE);
    AppDialog__set_submit_label((AppDialog *)dialog,ADD_DEVICE_SUBMIT_LABEL);
    AppDialog__set_show_callback((AppDialog *)dialog,priv_AddDeviceDialog__show_cb);
    AppDialog__set_destroy_callback((AppDialog*)dialog,priv_AddDeviceDialog__destroy);
    return dialog;
}

const char * AddDeviceDialog__get_host(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    return gtk_entry_get_text(GTK_ENTRY(elements->txthost));
}

const char * AddDeviceDialog__get_port(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    const char * port = gtk_entry_get_text(GTK_ENTRY(elements->txtport));
    if(!port || strlen(port) == 0){
        port = gtk_entry_get_placeholder_text(GTK_ENTRY(elements->txtport));
    }
    return port;
}

const char * AddDeviceDialog__get_protocol(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(elements->chkhttps))){
        return HTTPS_CONST_STRING;
    } else {
        return HTTP_CONST_STRING;
    }
}

const char * AddDeviceDialog__get_user(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    return gtk_entry_get_text(GTK_ENTRY(elements->txtuser));
}

const char * AddDeviceDialog__get_pass(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    return gtk_entry_get_text(GTK_ENTRY(elements->txtpass));
}

void priv_AddDeviceDialog__destroy(AppDialog * dialog){
    AddDeviceDialog* cdialog = (AddDeviceDialog*)dialog;
    free(cdialog->elements);
}

void priv_AddDeviceDialog__show_cb(AppDialogEvent * event){
    AddDeviceDialog * dialog = (AddDeviceDialog *) event->dialog;
    DialogElements * elements = (DialogElements *) dialog->elements;
    //Steal focus
    gtk_widget_grab_focus(elements->txthost);
    //Clear previous device uri
    gtk_entry_set_text(GTK_ENTRY(elements->txthost),"");
    gtk_entry_set_text(GTK_ENTRY(elements->txtport),"");
    gtk_entry_set_text(GTK_ENTRY(elements->txtuser),"");
    gtk_entry_set_text(GTK_ENTRY(elements->txtpass),"");
    gtk_widget_set_visible(elements->lblerr,FALSE);
    gtk_label_set_label(GTK_LABEL(elements->lblerr),"");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(elements->chkhttps),FALSE);
}