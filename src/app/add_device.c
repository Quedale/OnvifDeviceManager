#include "add_device.h"
#include "../queue/event_queue.h"

typedef struct { 
    GtkWidget * txtdeviceuri;
    GtkWidget * btnAdd;
    GtkWidget * btnCancel;
    GtkWidget * focusedWidget;
    int keysignalid;
} DialogElements;

AddDeviceEvent * AddDeviceEvent_copy(AddDeviceEvent * original){
   AddDeviceEvent * evt = malloc(sizeof(AddDeviceEvent));
   evt->dialog = original->dialog;
   evt->device_url = original->device_url;
   evt->user_data = original->user_data;
   return evt;
}

void add_device_dialog_add_cb (GtkWidget *widget, AddDeviceDialog * dialog) {
    AddDeviceEvent * evt = malloc(sizeof(AddDeviceEvent));
    DialogElements * elements = (DialogElements*) dialog->private;
    evt->dialog = dialog;
    evt->user_data = dialog->user_data;
    evt->device_url = (char *) gtk_entry_get_text(GTK_ENTRY(elements->txtdeviceuri));

    (*(dialog->add_callback))(evt);
    free(evt);
}

void add_device_dialog_cancel_cb (GtkWidget *widget, AddDeviceDialog * dialog) {
    (*(dialog->cancel_callback))(dialog);
    AddDeviceDialog__hide(dialog);
}

gboolean is_dialog_focused(AddDeviceDialog * dialog){
    DialogElements * elements = (DialogElements*) dialog->private;
    if(gtk_widget_has_focus(elements->txtdeviceuri) ||
        gtk_widget_has_focus(elements->btnAdd) ||
        gtk_widget_has_focus(elements->btnCancel) ){
            return TRUE;
    }   
    return FALSE;
}

gboolean add_device_dialog_keypress_function (GtkWidget *widget, GdkEventKey *event, AddDeviceDialog * dialog) {
    if(!is_dialog_focused(dialog)){
        return FALSE;
    }
    
    DialogElements * elements = (DialogElements*) dialog->private;
    if (event->keyval == GDK_KEY_Escape){
        add_device_dialog_cancel_cb(widget,dialog);
        return TRUE;
    } else if ((event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return) 
        && !gtk_widget_has_focus(elements->btnCancel)
        && !gtk_widget_has_focus(elements->btnAdd)){
        add_device_dialog_add_cb(widget,dialog);
        return TRUE;
    }
    return FALSE;
}

void add_device_dialog_set_nofocus(GtkWidget *widget, GtkWidget * dialog_root)
{
    if(widget != dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

void add_device_dialog_set_onlyfocus (GtkWidget *widget, GtkWidget * dialog_root)
{
    if(widget == dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

void AddDeviceDialog__show (AddDeviceDialog * dialog, void (*add_callback)(AddDeviceEvent *), void (*cancel_callback)(AddDeviceDialog *), void * user_data){
    if(dialog->visible == 1){
        return;
    }

    printf("AddDeviceDialog__show\n");
    dialog->visible = 1;

    //Set input userdata for current dialog session
    dialog->user_data = user_data;
    dialog->cancel_callback = cancel_callback;
    dialog->add_callback = add_callback;

    //Conncet keyboard handler. ESC = Cancel, Enter/Return = Login
    GtkWidget *window = gtk_widget_get_toplevel (dialog->root);
    DialogElements * elements = (DialogElements *) dialog->private; 
    if (gtk_widget_is_toplevel (window)){  
        elements->focusedWidget = gtk_window_get_focus(GTK_WINDOW(window));
        gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
        elements->keysignalid = g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (add_device_dialog_keypress_function), dialog);
    }

    //Prevent overlayed widget from getting focus
    GtkWidget *parent = gtk_widget_get_parent(dialog->root);
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)add_device_dialog_set_onlyfocus, dialog->root);
    
    //Steal focus
    gtk_widget_grab_focus(elements->txtdeviceuri);
    
    //Clear previous device uri
    gtk_entry_set_text(GTK_ENTRY(elements->txtdeviceuri),"");

    gtk_widget_set_visible(dialog->root,TRUE);

}

void AddDeviceDialog__hide (AddDeviceDialog * dialog){
    if(dialog->visible == 0){
        return;
    }
    printf("AddDeviceDialog__hide\n");
    dialog->visible = 0;

    //Disconnect keyboard handler.
    GtkWidget *window = gtk_widget_get_toplevel (dialog->root);
    DialogElements * elements = (DialogElements *) dialog->private; 
    if (gtk_widget_is_toplevel (window)){  
        g_signal_handler_disconnect (G_OBJECT (window), elements->keysignalid);
        elements->keysignalid = 0;
    }

    //Restore focus capabilities to previous widget
    GtkWidget *parent = gtk_widget_get_parent(dialog->root);
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)add_device_dialog_set_nofocus, dialog->root);

    //Restore stolen focus
    gtk_widget_grab_focus(elements->focusedWidget);
    gtk_widget_set_visible(dialog->root,FALSE);
}

GtkWidget * create_add_device_buttons(AddDeviceDialog * dialog){
    GtkWidget * label;
    GtkWidget * grid = gtk_grid_new ();
    g_object_set (grid, "margin", 5, NULL);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

    DialogElements * elements = (DialogElements *) dialog->private; 

    elements->btnAdd = gtk_button_new ();
    label = gtk_label_new("Add");
    g_object_set (elements->btnAdd, "margin-end", 10, NULL);
    gtk_container_add (GTK_CONTAINER (elements->btnAdd), label);
    gtk_widget_set_opacity(elements->btnAdd,1);
    g_signal_connect (elements->btnAdd, "clicked", G_CALLBACK (add_device_dialog_add_cb), dialog);
    
    gtk_grid_attach (GTK_GRID (grid), elements->btnAdd, 1, 0, 1, 1);

    elements->btnCancel = gtk_button_new ();
    label = gtk_label_new("Cancel");
    gtk_container_add (GTK_CONTAINER (elements->btnCancel), label);
    gtk_widget_set_opacity(elements->btnCancel,1);
    gtk_grid_attach (GTK_GRID (grid), elements->btnCancel, 2, 0, 1, 1);
    g_signal_connect (elements->btnCancel, "clicked", G_CALLBACK (add_device_dialog_cancel_cb), dialog);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);

    return grid;
}

GtkWidget * create_add_device_panel(AddDeviceDialog * dialog){
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();
    DialogElements * elements = (DialogElements *) dialog->private;

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


    gtk_grid_attach (GTK_GRID (grid), create_add_device_buttons(dialog), 0, 2, 2, 1);

    return grid;
}

GtkWidget * create_add_device_dialog(AddDeviceDialog * dialog){
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();

    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    //Add title strip
    widget = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (widget), "<span size=\"x-large\">Onvif Add Device</span>");
    g_object_set (widget, "margin", 10, NULL);
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);
    
    //Lightgrey background for the title strip
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; border-radius: 10px; background: linear-gradient(to top, @theme_bg_color);}",-1,NULL); 
    context = gtk_widget_get_style_context(grid);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_grid_attach (GTK_GRID (grid), create_add_device_panel(dialog), 0, 1, 1, 1);

    g_object_unref (cssProvider);  

    return grid;
}

void AddDeviceDialog__root_panel_show_cb (GtkWidget* self, AddDeviceDialog * dialog){
    if(!dialog->visible){
        gtk_widget_set_visible(self,FALSE);
    } else {
        gtk_widget_set_visible(self,TRUE);
    }
}

AddDeviceDialog * AddDeviceDialog__create(){
    AddDeviceDialog * dialog = malloc(sizeof(AddDeviceDialog));
    dialog->private = malloc(sizeof(DialogElements));
    GtkWidget * empty;
    dialog->root = gtk_grid_new ();
    dialog->visible = 0;

    GtkCssProvider * cssProvider;
    GtkStyleContext * context;
    
    //See-through overlay background
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black; opacity:0.3;}",-1,NULL); 

    //Remove this to align left
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (dialog->root), empty, 0, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align rights
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (dialog->root), empty, 2, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align top
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (dialog->root), empty, 1, 0, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align bot
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (dialog->root), empty, 1, 2, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    gtk_grid_attach (GTK_GRID (dialog->root), create_add_device_dialog(dialog), 1, 1, 1, 1);

    g_object_unref (cssProvider);  
    
    g_signal_connect (G_OBJECT (dialog->root), "show", G_CALLBACK (AddDeviceDialog__root_panel_show_cb), dialog);
    
    return dialog;
}

void AddDeviceDialog__destroy(AddDeviceDialog* dialog){
    free(dialog->private);
    free(dialog);
}