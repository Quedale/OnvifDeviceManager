#include "clogger.h"
#include "portable_thread.h"
#include "../src/app/omgr_device.h"

static void delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
}

static void device_profile_clicked (OnvifMgrDevice *self){
    printf("profile clicked!\n");
}

void device_widget_destroy(GtkWidget * widget, gpointer user_data){
    gtk_widget_destroy(widget);
}

static void device_destroy_clicked (OnvifMgrDevice *self, GtkBox * vbox){
    printf("destroy clicked!\n");
    gtk_container_foreach (GTK_CONTAINER (vbox), (GtkCallback)device_widget_destroy, NULL);
}

static void device_profile_changed (OnvifMgrDevice *self){
    printf("profile changed!\n");
}

int main(int argc, char *argv[])
{
    /* Initialize GTK */
    gtk_init (&argc, &argv);
    c_log_set_level(C_ALL_E);
    c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);
    
    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "OnvifMgrDevice Widget demo");

    OnvifDevice * device = OnvifDevice__create("http://61.216.97.157:16887/onvif/device_service");
    GtkWidget * omgr_device = OnvifMgrDevice__new(NULL, device,"Device Name", "Device hardware", "Device location");
    g_signal_connect (G_OBJECT(omgr_device), "profile-clicked", G_CALLBACK (device_profile_clicked), NULL);
    g_signal_connect (G_OBJECT(omgr_device), "profile-changed", G_CALLBACK (device_profile_changed), NULL);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window),vbox);

    GtkWidget * listbox = gtk_list_box_new ();
    gtk_widget_set_vexpand (listbox, TRUE);
    gtk_widget_set_hexpand (listbox, FALSE);
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox), GTK_SELECTION_SINGLE);
    
    GtkWidget *row = gtk_list_box_row_new ();
    gtk_container_add (GTK_CONTAINER (row), omgr_device);
    gtk_list_box_insert (GTK_LIST_BOX (listbox), row, -1);

    GtkWidget * scrollpanel = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrollpanel),listbox);
    gtk_box_pack_start(GTK_BOX(vbox), scrollpanel,     TRUE, TRUE, 0);

    GtkWidget * button = gtk_button_new_with_label("Destroy");
    g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK (device_destroy_clicked), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), button,     FALSE, FALSE, 0);

    gtk_window_set_default_size(GTK_WINDOW(window),100,100);
    gtk_widget_show_all (window);

    OnvifDevice__set_credentials(device,"demo","demo");
    OnvifDevice__authenticate(device);

    OnvifProfiles * profiles = OnvifMediaService__get_profiles(OnvifDevice__get_media_service(device));
    OnvifMgrDevice__set_profile(ONVIFMGR_DEVICE(omgr_device),OnvifProfiles__get_profile(profiles,0));
    OnvifProfiles__destroy(profiles);

    gtk_main ();

}