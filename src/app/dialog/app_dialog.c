#include "app_dialog.h"
#include "clist.h"
#include "clogger.h"

typedef struct { 
    GtkWidget * submit_btn;
    GtkWidget * cancel_btn;
    GtkWidget * title_lbl;
    GtkWidget * action_panel;
    GtkWidget * dummy;
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
void priv_AppDialog__create_buttons(AppDialog * dialog);
gboolean priv_AppDialog__keypress_function (GtkWidget *widget, GdkEventKey *event, AppDialog * dialog);
void priv_AppDialog__update_title(AppDialog * dialog);
void priv_AppDialog__update_submit_label(AppDialog * dialog);
void priv_AppDialog__toggle_actions_visibility(AppDialog * self);
int priv_AppDialog__has_focus(GtkWidget * widget);

void priv_AppDialolg__destroy(CObject * self) {
    AppDialog * dialog = (AppDialog *)self;
    if(dialog->destroy_callback){
        dialog->destroy_callback(dialog);
    }
    free(dialog->private);
    if(dialog->title){
        free(dialog->title);
    }
    if(dialog->submit_label){
        free(dialog->submit_label);
    }
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

void * AppDialog__get_user_data(AppDialog * self){
    return self->user_data;
}

void AppDialog__init(AppDialog* self, GtkWidget * (*create_ui)(AppDialogEvent *)) {
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_AppDialolg__destroy);
    C_TRACE("init");
    self->title = NULL;
    self->submit_label = NULL;
    self->visible = 0;
    self->action_visible = 1;
    self->root = NULL;
    self->submit_callback = NULL;
    self->cancel_callback = NULL;
    self->show_callback = NULL;
    self->user_data = NULL;
    self->closable = 1;
    self->submittable = 1;
    self->cancellable = 1;
    self->private = malloc(sizeof(AppDialogElements));
    self->create_ui = create_ui;
    priv_AppDialog__create_ui(self);
}

AppDialog * AppDialog__create(GtkWidget * (*create_ui)(AppDialogEvent *)){
    AppDialog* dialog = (AppDialog*) malloc(sizeof(AppDialog));
    AppDialog__init(dialog, create_ui);
    return dialog;
}

void AppDialog__show(AppDialog* dialog, void (*submit_callback)(AppDialogEvent *), void (*cancel_callback)(AppDialogEvent *), void * user_data){
    if(dialog->visible == 1){
        return;
    }

    C_DEBUG("show");
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

    priv_AppDialog__update_title(dialog);

    priv_AppDialog_show_event(dialog);

    gtk_widget_set_visible(dialog->root,TRUE);
    gtk_widget_hide(elements->dummy);
}

void AppDialog__hide(AppDialog* dialog){
    if(dialog->visible == 0){
        return;
    }
    C_DEBUG("hide");
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

int AppDialog__has_focus(AppDialog* dialog){
    return priv_AppDialog__has_focus(dialog->root);
}

int priv_AppDialog__has_focus(GtkWidget * widget){
    if(gtk_widget_has_focus(widget)){
        return 1;
    }

    if (!GTK_IS_CONTAINER(widget)){
        return 0;
    }

    GList * children = gtk_container_get_children(GTK_CONTAINER(widget));
    GList * tmp = children;
    while (tmp != NULL){
        GtkWidget * child = tmp->data;
        if(priv_AppDialog__has_focus(child)){
            g_list_free (children);
            return 1;
        }
        tmp = tmp->next;
    }
    g_list_free (children);
    return 0;
}

gboolean priv_AppDialog__keypress_function (GtkWidget *widget, GdkEventKey *event, AppDialog * dialog) {
    AppDialogElements * elements = (AppDialogElements*) dialog->private;
    if(!AppDialog__has_focus(dialog)){
        return FALSE;
    }

    if (event->keyval == GDK_KEY_Escape){
        priv_AppDialog_cancel_event(widget,dialog);
        return TRUE;
    } else if (dialog->action_visible &&
        (event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return) 
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
    gtk_widget_set_size_request (empty, 20,-1);
    gtk_grid_attach (GTK_GRID (self->root), empty, 0, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align rights
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_widget_set_size_request (empty, 20,-1);
    gtk_grid_attach (GTK_GRID (self->root), empty, 2, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align top
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_widget_set_size_request (empty, -1,20);
    gtk_grid_attach (GTK_GRID (self->root), empty, 1, 0, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align bot
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_widget_set_size_request (empty, -1,20);
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

    AppDialogElements * elements = (AppDialogElements *) dialog->private; 

    //Add title strip
    elements->title_lbl = gtk_label_new("");
    g_object_set (elements->title_lbl, "margin", 10, NULL);
    gtk_widget_set_hexpand (elements->title_lbl, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->title_lbl, 0, 0, 1, 1);

    elements->dummy = gtk_entry_new();
    g_object_set (elements->dummy, "margin-right", 10, NULL);
    gtk_widget_set_hexpand (elements->dummy, TRUE);
    gtk_grid_attach (GTK_GRID (grid), elements->dummy, 1, 0, 1, 1);

    //Lightgrey background for the title strip
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; border-radius: 10px; background: linear-gradient(to top, @theme_bg_color, @theme_bg_color);}",-1,NULL); 
    context = gtk_widget_get_style_context(grid);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    widget = priv_AppDialog_create_parent_ui(dialog);
    if(widget){
        gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
    }

    priv_AppDialog__create_buttons(dialog);
    gtk_grid_attach (GTK_GRID (grid), elements->action_panel, 0, 2, 1, 1);

    g_object_unref (cssProvider);  

    return grid;
}

void priv_AppDialog__attach_cancel_button(AppDialog *dialog){
    if(dialog->cancellable){
        AppDialogElements * elements = (AppDialogElements *) dialog->private; 
        elements->cancel_btn = gtk_button_new ();
        GtkWidget * label = gtk_label_new("Cancel");
        gtk_container_add (GTK_CONTAINER (elements->cancel_btn), label);
        gtk_grid_attach (GTK_GRID (elements->action_panel), elements->cancel_btn, 2, 0, 1, 1);
        g_signal_connect (elements->cancel_btn, "clicked", G_CALLBACK (priv_AppDialog_cancel_event), dialog);
    }
}

void priv_AppDialog__attach_submit_button(AppDialog *dialog){
    if(dialog->submittable){
        AppDialogElements * elements = (AppDialogElements *) dialog->private; 
        elements->submit_btn = gtk_button_new ();
        GtkWidget * label = gtk_label_new(dialog->submit_label);
        g_object_set (elements->submit_btn, "margin-end", 10, NULL);
        gtk_container_add (GTK_CONTAINER (elements->submit_btn), label);
        g_signal_connect (elements->submit_btn, "clicked", G_CALLBACK (priv_AppDialog_submit_event), dialog);
        gtk_grid_attach (GTK_GRID (elements->action_panel), elements->submit_btn, 1, 0, 1, 1);
    }
}

void priv_AppDialog__create_buttons(AppDialog * dialog){
    GtkWidget * label;
    AppDialogElements * elements = (AppDialogElements *) dialog->private; 
    elements->action_panel = gtk_grid_new ();
    g_object_set (elements->action_panel, "margin", 5, NULL);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (elements->action_panel), label, 0, 0, 1, 1);

    priv_AppDialog__attach_submit_button(dialog);

    priv_AppDialog__attach_cancel_button(dialog);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (elements->action_panel), label, 3, 0, 1, 1);
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
    if(!dialog->closable){
        return;
    }
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
    AppDialogElements * elements = (AppDialogElements*)dialog->private;
    gtk_widget_grab_focus(elements->dummy);
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

void priv_AppDialog__update_title(AppDialog * dialog){
    AppDialogElements * elements = (AppDialogElements *) dialog->private;
    if(elements && GTK_IS_WIDGET(elements->title_lbl)){
        char * input;
        if(dialog->title){
            input = dialog->title;
        } else {
            input = "Empty Title";
        }
        char * new_title = malloc(strlen(APPDIALOG_TITLE_PREFIX) + strlen(input) + strlen(APPDIALOG_TITLE_SUFFIX) +1);
        strcpy(new_title,APPDIALOG_TITLE_PREFIX);
        strcat(new_title,input);
        strcat(new_title,APPDIALOG_TITLE_SUFFIX);

        gtk_label_set_markup (GTK_LABEL (elements->title_lbl), new_title);
        free(new_title);
    }
}

void priv_AppDialog__update_submit_label(AppDialog * dialog){
    AppDialogElements * elements = (AppDialogElements *) dialog->private;
    if(elements && GTK_IS_WIDGET(elements->submit_btn)){
        gtk_button_set_label(GTK_BUTTON(elements->submit_btn), dialog->submit_label);
    }
}

void AppDialog__set_title(AppDialog * self, char * title){
    if(!title){
        if(self->title){
            free(self->title);
        }
        return;
    }
    if(!self->title){
        self->title = malloc(strlen(title)+1);
    } else {
        self->title = realloc(self->title,strlen(title)+1);
    }
    strcpy(self->title,title);
    priv_AppDialog__update_title(self);
}

void priv_AppDialog__toggle_actions_visibility(AppDialog * self){
    AppDialogElements * elements = (AppDialogElements *) self->private;
    if(elements && GTK_IS_WIDGET(elements->action_panel)){
        gtk_widget_set_visible(elements->action_panel,self->action_visible);
    }
}

void AppDialog__set_closable(AppDialog * self, int closable){
    self->closable = closable;
}

void AppDialog__set_submitable(AppDialog * self, int submitable){
    AppDialogElements * elements = (AppDialogElements*) self->private;
    self->submittable = submitable;
    if(GTK_IS_WIDGET(elements->submit_btn) && !submitable){
        gtk_widget_destroy(elements->submit_btn);
        elements->submit_btn = NULL;
    } else if(!GTK_IS_WIDGET(elements->submit_btn) && submitable){
        priv_AppDialog__attach_submit_button(self);
    }
}

void AppDialog__set_cancellable(AppDialog * self, int cancellable){
    AppDialogElements * elements = (AppDialogElements*) self->private;
    self->cancellable = cancellable;
    if(GTK_IS_WIDGET(elements->cancel_btn) && !cancellable){
        gtk_widget_destroy(elements->cancel_btn);
        elements->cancel_btn = NULL;
    } else if(!GTK_IS_WIDGET(elements->cancel_btn) && cancellable){
        priv_AppDialog__attach_cancel_button(self);
    }
}

void AppDialog__hide_actions(AppDialog * self){
    self->action_visible = 0;
    priv_AppDialog__toggle_actions_visibility(self);
}

void AppDialog__show_actions(AppDialog * self){
    self->action_visible = 1;
    priv_AppDialog__toggle_actions_visibility(self);
}

void AppDialog__set_submit_label(AppDialog* self, char * label){
    if(!label){
        if(self->submit_label){
            free(self->submit_label);
        }
        return;
    }
    if(!self->submit_label){
        self->submit_label = malloc(strlen(label)+1);
    } else {
        self->submit_label = realloc(self->submit_label,strlen(label)+1);
    }
    strcpy(self->submit_label,label);
    priv_AppDialog__update_submit_label(self);
}