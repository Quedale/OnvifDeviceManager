#include "clogger.h"
#include "portable_thread.h"
#include "../src/app/dialog/omgr_credentials_dialog.h"
#include "../src/app/dialog/omgr_add_dialog.h"
#include "../src/app/omgr_device_row.h"

static void delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
}

void dialog_login_cb(OnvifMgrAppDialog * app_dialog){
    char * error = NULL;
    g_object_get (G_OBJECT (app_dialog), "error", &error, NULL);
    if(!error || !strlen(error)){
        g_object_set (app_dialog, "error", "TEST", NULL);
    } else {
        g_object_set (app_dialog, "error", NULL, NULL);
    }
}

void demo_clicked (GtkButton* widget, OnvifMgrAppDialog * dialog){
    C_WARN(" ------------------ demo clicked ----------------- ");
    gtk_widget_show(GTK_WIDGET(dialog));
}

int main(int argc, char *argv[])
{
    /* Initialize GTK */
    gtk_init (&argc, &argv);
    c_log_set_level(C_ALL_E);
    c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);


    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "OnvifCredentialsDialog Widget demo");

    OnvifDevice * device = OnvifDevice__create("http://61.216.97.157:16887/onvif/device_service");
    GtkWidget * omgr_device = OnvifMgrDeviceRow__new(NULL, device,"Device Name", "Device hardware", "Device location");

    GtkWidget * overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window),overlay);

    GtkWidget * button = gtk_button_new_with_label("Show Dialog");
    GtkWidget * entry = gtk_entry_new();
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE, 0);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),GTK_WIDGET(hbox));

    // OnvifMgrCredentialsDialog * dialog = OnvifMgrCredentialsDialog__new();
    OnvifMgrAddDialog * dialog = OnvifMgrAddDialog__new();
    g_object_set (dialog,   "userdata", omgr_device, 
                            "self-destroy", FALSE, //Setting self-destroy to false so that we can re-use the dialog after its closed
                        NULL);

    //Set signals
    g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK (demo_clicked), dialog);
    g_signal_connect (G_OBJECT (dialog), "submit", G_CALLBACK (dialog_login_cb), omgr_device);
    // g_signal_connect (G_OBJECT (dialog), "cancel", G_CALLBACK (OnvifApp__cred_dialog_cancel_cb), omgr_device);
    
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),GTK_WIDGET(dialog));


    gtk_window_set_default_size(GTK_WINDOW(window),400,400);
    gtk_widget_show_all (window);
    gtk_widget_hide(GTK_WIDGET(dialog));
    gtk_widget_grab_focus(button);
    gtk_main ();
}