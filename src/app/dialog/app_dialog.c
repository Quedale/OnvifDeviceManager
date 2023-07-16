#include "app_dialog.h"
#include "../../oo/clist.h"

typedef struct { 
    GtkWidget * submit_btn;
    GtkWidget * cancel_btn;
    
    GtkWidget * focusedWidget;
    int keysignalid;
} AppDialogElements;

void priv_AppDialog__destroy(CObject * self);
void priv_AppDialog__set_onlyfocus (GtkWidget *widget, GtkWidget * dialog_root);
void priv_AppDialog__set_nofocus(GtkWidget *widget, GtkWidget * dialog_root);
void priv_AppDialog__create_ui(AppDialog * self);
void priv_AppDialog__root_panel_show_cb (GtkWidget* self, AppDialog * dialog);
void priv_AppDialog_submit_event (GtkWidget *widget, AppDialog * dialog);
void priv_AppDialog_cancel_event (GtkWidget *widget, AppDialog * dialog);
void priv_AppDialog_show_event (AppDialog * dialog);
GtkWidget * priv_AppDialog__create_panel(AppDialog * dialog);
GtkWidget * priv_AppDialog_create_parent_ui (AppDialog * dialog);
GtkWidget * priv_AppDialog__create_buttons(AppDialog * dialog);
gboolean priv_AppDialog__keypress_function (GtkWidget *widget, GdkEventKey *event, AppDialog * dialog);

void priv_AppDialolg__destroy(CObject * self) {
    AppDialog * dialog = (AppDialog *)self;
    if(dialog->destroy_callback){
        dialog->destroy_callback(dialog);
    }
    free(dialog->private);
}

AppDialogEvent * AppDialogEvent_copy(AppDialogEvent * original){
   AppDialogEvent * evt = malloc(sizeof(AppDialogEvent));
   evt->dialog = original->dialog;
   evt->type = original->type;
   evt->user_data = original->user_data;
   return evt;
}

void AppDialog__set_show_callback(AppDialog* dialog, void (*show_callback)(AppDialogEvent *)){
    dialog->show_callback = show_callback;
}

void AppDialog__set_destroy_callback(AppDialog* self, void (*destroy_callback)(AppDialog *)){
    self->destroy_callback = destroy_callback;
}

void AppDialog__init(AppDialog* self, char * title, char * submit_label, GtkWidget * (*create_ui)(AppDialogEvent *)) {
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_AppDialolg__destroy);
    self->title = title;
    self->submit_label = submit_label;
    self->visible = 0;
    self->root = NULL;
    self->submit_callback = NULL;
    self->cancel_callback = NULL;
    self->user_data = NULL;
    self->private = malloc(sizeof(AppDialogElements));
    self->create_ui = create_ui;
    priv_AppDialog__create_ui(self);
}

AppDialog * AppDialog__create(char * title, char * submit_label, GtkWidget * (*create_ui)(AppDialogEvent *)){
    AppDialog* dialog = (AppDialog*) malloc(sizeof(AppDialog));
    AppDialog__init(dialog,title,submit_label, create_ui);
    return dialog;
}

void AppDialog__show(AppDialog* dialog, void (*submit_callback)(AppDialogEvent *), void (*cancel_callback)(AppDialogEvent *), void * user_data){
    if(dialog->visible == 1){
        return;
    }

    printf("AppDialog__show\n");
    dialog->visible = 1;

    //Set input userdata for current dialog session
    dialog->user_data = user_data;
    dialog->cancel_callback = cancel_callback;
    dialog->submit_callback = submit_callback;

    //Conncet keyboard handler. ESC = Cancel, Enter/Return = Login
    GtkWidget *window = gtk_widget_get_toplevel (dialog->root);
    AppDialogElements * elements = (AppDialogElements *) dialog->private; 
    if (gtk_widget_is_toplevel (window)){  
        elements->focusedWidget = gtk_window_get_focus(GTK_WINDOW(window));
        gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
        elements->keysignalid = g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (priv_AppDialog__keypress_function), dialog);
    }

    //Prevent overlayed widget from getting focus
    GtkWidget *parent = gtk_widget_get_parent(dialog->root);
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)priv_AppDialog__set_onlyfocus, dialog->root);

    priv_AppDialog_show_event(dialog);

    gtk_widget_set_visible(dialog->root,TRUE);
}

void AppDialog__hide(AppDialog* dialog){
    if(dialog->visible == 0){
        return;
    }
    printf("AddDeviceDialog__hide\n");
    dialog->visible = 0;

    //Disconnect keyboard handler.
    GtkWidget *window = gtk_widget_get_toplevel (dialog->root);
    AppDialogElements * elements = (AppDialogElements *) dialog->private; 
    if (gtk_widget_is_toplevel (window)){  
        g_signal_handler_disconnect (G_OBJECT (window), elements->keysignalid);
        elements->keysignalid = 0;
    }

    //Restore focus capabilities to previous widget
    GtkWidget *parent = gtk_widget_get_parent(dialog->root);
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)priv_AppDialog__set_nofocus, dialog->root);

    //Restore stolen focus
    gtk_widget_grab_focus(elements->focusedWidget);

    gtk_widget_set_visible(dialog->root,FALSE);
}

void AppDialog__add_to_overlay(AppDialog * self, GtkOverlay * overlay){
    gtk_overlay_add_overlay(overlay,self->root);
}

void priv_AppDialog__set_nofocus(GtkWidget *widget, GtkWidget * dialog_root){
    if(widget != dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

void priv_AppDialog__set_onlyfocus (GtkWidget *widget, GtkWidget * dialog_root){
    if(widget == dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

gboolean priv_AppDialog__keypress_function (GtkWidget *widget, GdkEventKey *event, AppDialog * dialog) {
    AppDialogElements * elements = (AppDialogElements*) dialog->private;
    if (event->keyval == GDK_KEY_Escape){
        priv_AppDialog_cancel_event(widget,dialog);
        return TRUE;
    } else if ((event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return) 
        && !gtk_widget_has_focus(elements->cancel_btn)
        && !gtk_widget_has_focus(elements->submit_btn)){
        priv_AppDialog_submit_event(widget,dialog);
        return TRUE;
    }
    return FALSE;
}

void priv_AppDialog__create_ui(AppDialog * self){
    GtkWidget * empty;
    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    self->root = gtk_grid_new ();

   //See-through overlay background
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black; opacity:0.3;}",-1,NULL); 

    //Remove this to align left
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self->root), empty, 0, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align rights
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self->root), empty, 2, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align top
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self->root), empty, 1, 0, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align bot
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self->root), empty, 1, 2, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    gtk_grid_attach (GTK_GRID (self->root), priv_AppDialog__create_panel(self), 1, 1, 1, 1);

    g_object_unref (cssProvider);  
    
    g_signal_connect (G_OBJECT (self->root), "show", G_CALLBACK (priv_AppDialog__root_panel_show_cb), self);

}

GtkWidget * priv_AppDialog__create_panel(AppDialog * dialog){
    GtkWidget * widget;
    GtkWidget * grid = gtk_grid_new ();

    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    //Add title strip
    widget = gtk_label_new("");
    char * new_title = malloc(strlen(APPDIALOG_TITLE_PREFIX) + strlen(dialog->title) + strlen(APPDIALOG_TITLE_SUFFIX) +1);
    strcpy(new_title,APPDIALOG_TITLE_PREFIX);
    strcat(new_title,dialog->title);
    strcat(new_title,APPDIALOG_TITLE_SUFFIX);

    gtk_label_set_markup (GTK_LABEL (widget), new_title);
    g_object_set (widget, "margin", 10, NULL);
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);
    free(new_title);
    
    //Lightgrey background for the title strip
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; border-radius: 10px; background: linear-gradient(to top, @theme_bg_color, @theme_bg_color);}",-1,NULL); 
    context = gtk_widget_get_style_context(grid);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    widget = priv_AppDialog_create_parent_ui(dialog);
    if(widget){
        gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
    }

    widget = priv_AppDialog__create_buttons(dialog);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);

    g_object_unref (cssProvider);  

    return grid;
}

GtkWidget * priv_AppDialog__create_buttons(AppDialog * dialog){
    GtkWidget * label;
    GtkWidget * grid = gtk_grid_new ();
    g_object_set (grid, "margin", 5, NULL);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

    AppDialogElements * elements = (AppDialogElements *) dialog->private; 

    elements->submit_btn = gtk_button_new ();
    label = gtk_label_new(dialog->submit_label);
    g_object_set (elements->submit_btn, "margin-end", 10, NULL);
    gtk_container_add (GTK_CONTAINER (elements->submit_btn), label);
    gtk_widget_set_opacity(elements->submit_btn,1);
    g_signal_connect (elements->submit_btn, "clicked", G_CALLBACK (priv_AppDialog_submit_event), dialog);
    
    gtk_grid_attach (GTK_GRID (grid), elements->submit_btn, 1, 0, 1, 1);

    elements->cancel_btn = gtk_button_new ();
    label = gtk_label_new("Cancel");
    gtk_container_add (GTK_CONTAINER (elements->cancel_btn), label);
    gtk_widget_set_opacity(elements->cancel_btn,1);
    gtk_grid_attach (GTK_GRID (grid), elements->cancel_btn, 2, 0, 1, 1);
    g_signal_connect (elements->cancel_btn, "clicked", G_CALLBACK (priv_AppDialog_cancel_event), dialog);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);

    return grid;
}

void priv_AppDialog__root_panel_show_cb (GtkWidget* self, AppDialog * dialog){
    if(!dialog->visible){
        gtk_widget_set_visible(self,FALSE);
    } else {
        gtk_widget_set_visible(self,TRUE);
    }
}

void priv_AppDialog_submit_event (GtkWidget *widget, AppDialog * dialog) {
    if(dialog->submit_callback){
        AppDialogEvent * evt = malloc(sizeof(AppDialogEvent));
        evt->dialog = dialog;
        evt->type = APP_DIALOG_SUBMIT_EVENT;
        evt->user_data = dialog->user_data;

        (*(dialog->submit_callback))(evt);
        free(evt);
    }
}

void priv_AppDialog_cancel_event (GtkWidget *widget, AppDialog * dialog) {
    if(dialog->cancel_callback){
        AppDialogEvent * evt = malloc(sizeof(AppDialogEvent));
        evt->dialog = dialog;
        evt->type = APP_DIALOG_CANCEL_EVENT;
        evt->user_data = dialog->user_data;

        (*(dialog->cancel_callback))(evt);
        free(evt);
    }
    AppDialog__hide(dialog);
}

void priv_AppDialog_show_event (AppDialog * dialog) {
    if(dialog->show_callback){
        AppDialogEvent * evt = malloc(sizeof(AppDialogEvent));
        evt->dialog = dialog;
        evt->type = APP_DIALOG_SHOW_EVENT;
        evt->user_data = dialog->user_data;
        (*(dialog->show_callback))(evt);
        free(evt);
    }
}

GtkWidget * priv_AppDialog_create_parent_ui (AppDialog * dialog) {
    GtkWidget * widget = NULL;
    if(dialog->create_ui){
        AppDialogEvent * evt = malloc(sizeof(AppDialogEvent));
        evt->dialog = dialog;
        evt->type = APP_DIALOG_UI_EVENT;
        evt->user_data = dialog->user_data;

        widget = (*(dialog->create_ui))(evt);
        free(evt);
    }

    return widget;
}