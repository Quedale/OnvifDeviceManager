#include "credentials_input.h"
#include "../queue/event_queue.h"

typedef struct { 
    GtkWidget * txtuser;
    GtkWidget * txtpassword;
    GtkWidget * focusedWidget;
    int keysignalid;
} DialogElements;

LoginEvent * LoginEvent_copy(LoginEvent * original){
   LoginEvent * evt = malloc(sizeof(LoginEvent));
   evt->dialog = original->dialog;
   evt->pass = original->pass;
   evt->user = original->user;
   evt->user_data = original->user_data;
   return evt;
}


void cedentials_dialog_login_cb (GtkWidget *widget, CredentialsDialog * dialog) {
    LoginEvent * evt = malloc(sizeof(LoginEvent));
    DialogElements * elements = (DialogElements*) dialog->private;
    evt->dialog = dialog;
    evt->user_data = dialog->user_data;
    evt->user = (char *) gtk_entry_get_text(GTK_ENTRY(elements->txtuser));
    evt->pass = (char *) gtk_entry_get_text(GTK_ENTRY(elements->txtpassword));

    (*(dialog->login_callback))(evt);
    free(evt);
}

void cedentials_dialog_cancel_cb (GtkWidget *widget, CredentialsDialog * dialog) {
    (*(dialog->cancel_callback))(dialog);
    CredentialsDialog__hide(dialog);
}

gboolean credentials_dialog_keypress_function (GtkWidget *widget, GdkEventKey *event, CredentialsDialog * dialog) {
    if (event->keyval == GDK_KEY_Escape){
        cedentials_dialog_cancel_cb(widget,dialog);
        return TRUE;
    } else if (event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return){
        cedentials_dialog_login_cb(widget,dialog);
        return TRUE;
    }
    return FALSE;
}

void
credentials_dialog_set_nofocus(GtkWidget *widget, GtkWidget * dialog_root)
{
    if(widget != dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}
void
credentials_dialog_set_onlyfocus (GtkWidget *widget, GtkWidget * dialog_root)
{
    if(widget == dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

void CredentialsDialog__show (CredentialsDialog * dialog, void * user_data){
    if(dialog->visible == 1){
        return;
    }
    printf("CredentialsDialog__show\n");
    dialog->visible = 1;

    //Set input userdata for current dialog session
    dialog->user_data = user_data;

    //Conncet keyboard handler. ESC = Cancel, Enter/Return = Login
    GtkWidget *window = gtk_widget_get_toplevel (dialog->root);
    DialogElements * elements = (DialogElements *) dialog->private; 
    if (gtk_widget_is_toplevel (window)){  
        elements->focusedWidget = gtk_window_get_focus(GTK_WINDOW(window));
        gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
        elements->keysignalid = g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (credentials_dialog_keypress_function), dialog);
    }

    //Prevent overlayed widget from getting focus
    GtkWidget *parent = gtk_widget_get_parent(dialog->root);
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)credentials_dialog_set_onlyfocus, dialog->root);
    
    //Steal focus
    gtk_widget_grab_focus(elements->txtuser);
    
    //Clear previous credentials
    gtk_entry_set_text(GTK_ENTRY(elements->txtuser),"");
    gtk_entry_set_text(GTK_ENTRY(elements->txtpassword),"");

    gtk_widget_set_visible(dialog->root,TRUE);
}

void CredentialsDialog__hide (CredentialsDialog * dialog){
    if(dialog->visible == 0){
        return;
    }
    printf("CredentialsDialog__hide\n");
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
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)credentials_dialog_set_nofocus, dialog->root);

    //Restore stolen focus
    gtk_widget_grab_focus(elements->focusedWidget);
    gtk_widget_set_visible(dialog->root,FALSE);
}

GtkWidget * create_credentials_buttons(CredentialsDialog * dialog){
    GtkWidget * widget;
    GtkWidget * label;
    GtkWidget * grid = gtk_grid_new ();
    g_object_set (grid, "margin", 5, NULL);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

    widget = gtk_button_new ();
    label = gtk_label_new("Login");
    g_object_set (widget, "margin-end", 10, NULL);
    gtk_container_add (GTK_CONTAINER (widget), label);
    gtk_widget_set_opacity(widget,1);
    g_signal_connect (widget, "clicked", G_CALLBACK (cedentials_dialog_login_cb), dialog);
    
    gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 1);

    widget = gtk_button_new ();
    label = gtk_label_new("Cancel");
    gtk_container_add (GTK_CONTAINER (widget), label);
    gtk_widget_set_opacity(widget,1);
    gtk_grid_attach (GTK_GRID (grid), widget, 2, 0, 1, 1);
    g_signal_connect (widget, "clicked", G_CALLBACK (cedentials_dialog_cancel_cb), dialog);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);

    return grid;
}

GtkWidget * create_credentials_panel(CredentialsDialog * dialog){
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();
    DialogElements * elements = (DialogElements *) dialog->private;
    
    widget = gtk_label_new("Username :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

    elements->txtuser = gtk_entry_new();
    gtk_widget_set_hexpand (elements->txtuser, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtuser, 1, 0, 1, 1);
    
    widget = gtk_label_new("Password :");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

    elements->txtpassword = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(elements->txtpassword),FALSE);
    gtk_widget_set_hexpand (elements->txtpassword, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->txtpassword, 1, 1, 1, 1);


    gtk_grid_attach (GTK_GRID (grid), create_credentials_buttons(dialog), 0, 2, 2, 1);

    return grid;
}

GtkWidget * create_credentials_dialog(CredentialsDialog * dialog){
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();
        
    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    //Add title strip
    widget = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (widget), "<span size=\"x-large\">Onvif Device Authentication</span>");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

    //Lightgrey background for the title strip
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:#D3D3D3;}",-1,NULL); 
    context = gtk_widget_get_style_context(grid);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_grid_attach (GTK_GRID (grid), create_credentials_panel(dialog), 0, 1, 1, 1);

    return grid;
}


void CredentialsDialog__root_panel_show_cb (GtkWidget* self, CredentialsDialog * dialog){
    if(!dialog->visible){
        gtk_widget_set_visible(self,FALSE);
    } else {
        gtk_widget_set_visible(self,TRUE);
    }
}

CredentialsDialog * CredentialsDialog__create(void (*login_callback)(LoginEvent *), void (*cancel_callback)(CredentialsDialog *)){
    CredentialsDialog * dialog = malloc(sizeof(CredentialsDialog));
    dialog->cancel_callback = cancel_callback;
    dialog->login_callback = login_callback;
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
    
    gtk_grid_attach (GTK_GRID (dialog->root), create_credentials_dialog(dialog), 1, 1, 1, 1);

    g_object_unref (cssProvider);  
    
    g_signal_connect (G_OBJECT (dialog->root), "show", G_CALLBACK (CredentialsDialog__root_panel_show_cb), dialog);
    
    return dialog;
}

void CredentialsDialog__destroy(CredentialsDialog* dialog){
    free(dialog->private);
    free(dialog);
}