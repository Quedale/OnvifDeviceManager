#include "add_device.h"
#include "../../queue/event_queue.h"
#include "app_dialog.h"

#define ADD_DEVICE_TITLE "ONVIF Add Device"
#define ADD_DEVICE_SUBMIT_LABEL "Add"

typedef struct { 
    GtkWidget * txtdeviceuri;
} DialogElements;

GtkWidget * priv_AddDeviceDialog__create_iu(AppDialogEvent * event);
void priv_AddDeviceDialog__show_cb(AppDialogEvent *);
void priv_AddDeviceDialog__destroy(AppDialog * dialog);

GtkWidget * priv_AddDeviceDialog__create_iu(AppDialogEvent * event){
    AddDeviceDialog * dialog = (AddDeviceDialog *) event->dialog; 
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();
    DialogElements * elements = (DialogElements *) dialog->elements;

    widget = gtk_label_new("The device URI is the ONVIF device entry point.\n(e.g. https://192.168.1.2:8443/onvif/device_service)");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 2, 1);
    gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_CENTER);

    widget = gtk_label_new("Device URI :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
    
    elements->txtdeviceuri = gtk_entry_new();
    g_object_set (elements->txtdeviceuri, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->txtdeviceuri, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtdeviceuri, 1, 1, 1, 1);

    return grid;
}

AddDeviceDialog * AddDeviceDialog__create(){
    AddDeviceDialog * dialog = malloc(sizeof(AddDeviceDialog));
    dialog->elements = malloc(sizeof(DialogElements));

    AppDialog__init((AppDialog *)dialog, priv_AddDeviceDialog__create_iu);
    CObject__set_allocated((CObject *) dialog);
    AppDialog__set_title((AppDialog *)dialog,ADD_DEVICE_TITLE);
    AppDialog__set_submit_label((AppDialog *)dialog,ADD_DEVICE_SUBMIT_LABEL);
    AppDialog__set_show_callback((AppDialog *)dialog,priv_AddDeviceDialog__show_cb);
    AppDialog__set_destroy_callback((AppDialog*)dialog,priv_AddDeviceDialog__destroy);
    return dialog;
}

const char * AddDeviceDialog__get_device_uri(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements *) dialog->elements;
    return gtk_entry_get_text(GTK_ENTRY(elements->txtdeviceuri));
}

void priv_AddDeviceDialog__destroy(AppDialog * dialog){
    AddDeviceDialog* cdialog = (AddDeviceDialog*)dialog;
    free(cdialog->elements);
}

void priv_AddDeviceDialog__show_cb(AppDialogEvent * event){
    AddDeviceDialog * dialog = (AddDeviceDialog *) event->dialog;
    DialogElements * elements = (DialogElements *) dialog->elements;
    //Steal focus
    gtk_widget_grab_focus(elements->txtdeviceuri);
    //Clear previous device uri
    gtk_entry_set_text(GTK_ENTRY(elements->txtdeviceuri),"");
}