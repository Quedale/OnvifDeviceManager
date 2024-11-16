#include "omgr_app_dialog.h"
#include <gtk/gtk.h>
#include "clogger.h"
#include "../../animations/gtk/gtk_dotted_slider_widget.h"

#define ONVIFMGR_APPDIALOG_TITLE_PREFIX "<span size=\"x-large\">"
#define ONVIFMGR_APPDIALOG_TITLE_SUFFIX "</span>"
static const char * default_title_label = "AppDialog Default Title";
static const char * default_submit_label = "Submit";

enum {
  SIGNAL_SUBMIT,
  SIGNAL_CANCEL,
  SIGNAL_SHOWING,
  LAST_SIGNAL
};

enum
{
  PROP_ACTIONVISIBLE = 1,
  PROP_CLOSABLE = 2,
  PROP_CANCELLABLE = 3,
  PROP_SUBMITTABLE = 4,
  PROP_TITLELABEL = 5,
  PROP_SUBMITLABEL = 6,
  PROP_USERDATA = 7,
  PROP_SELFDESTROY = 8,
  PROP_ERROR = 9,
  N_PROPERTIES
};

typedef struct _OnvifMgrAppDialogPrivate       OnvifMgrAppDialogPrivate;

struct _OnvifMgrAppDialogPrivate {
    GtkWidget * panel;
    GtkWidget * panel_decor;
    GtkWidget * loading_panel;
    GtkWidget * submit_btn;
    GtkWidget * cancel_btn;
    GtkWidget * title_lbl;
    GtkWidget * action_panel;
    GtkWidget * innerFocusedWidget;
    GtkWidget * focusedWidget;
    GtkWidget * lblerr;
    int keysignalid;

    gboolean self_destroy;
    gboolean closable;
    char * title;
    gpointer user_data;
};

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrAppDialog, OnvifMgrAppDialog_, GTK_TYPE_GRID)


/*
    TODO Combine submit methods into one
*/
static void 
OnvifMgrAppDialog__submit_event (GtkWidget *widget, OnvifMgrAppDialog * self) {
    g_signal_emit (self, signals[SIGNAL_SUBMIT], 0);
}

static void 
OnvifMgrAppDialog__cancel_event (GtkWidget *widget, OnvifMgrAppDialog * self) {
    g_signal_emit (self, signals[SIGNAL_CANCEL], 0);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    if(!priv->closable){
        return;
    }

    OnvifMgrAppDialog__close(self);
}

/*
    TODO Combine attach methods into one
*/
static void 
OnvifMgrAppDialog__attach_cancel_button(OnvifMgrAppDialog *self){
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    priv->cancel_btn = gtk_button_new ();
    GtkWidget * label = gtk_label_new("Cancel");
    gtk_container_add (GTK_CONTAINER (priv->cancel_btn), label);
    gtk_grid_attach (GTK_GRID (priv->action_panel), priv->cancel_btn, 4, 0, 1, 1);
    g_signal_connect (priv->cancel_btn, "clicked", G_CALLBACK (OnvifMgrAppDialog__cancel_event), self);
}

static void 
OnvifMgrAppDialog__attach_submit_button(OnvifMgrAppDialog *self){
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    priv->submit_btn = gtk_button_new ();
    GtkWidget * label = gtk_label_new("");
    gtk_container_add (GTK_CONTAINER (priv->submit_btn), label);
    gtk_grid_attach (GTK_GRID (priv->action_panel), priv->submit_btn, 2, 0, 1, 1);
    g_signal_connect (priv->submit_btn, "clicked", G_CALLBACK (OnvifMgrAppDialog__submit_event), self);
}

static void 
OnvifMgrAppDialog__create_buttons(OnvifMgrAppDialog * self){
    GtkWidget * label;
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    priv->action_panel = gtk_grid_new ();
    gtk_grid_set_column_spacing(GTK_GRID (priv->action_panel),5);
    gtk_widget_set_margin_top(priv->action_panel,10);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (priv->action_panel), label, 0, 0, 1, 1);

    label = gtk_label_new("");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (priv->action_panel), label, 6, 0, 1, 1);
}

static GtkWidget * 
OnvifMgrAppDialog__dummy_create_ui(OnvifMgrAppDialog * self){
    C_ERROR("No create_ui method defined.");
    return gtk_label_new("Empty panel. Work In Progress...");
}

static GtkWidget * 
OnvifMgrAppDialog__create_panel(OnvifMgrAppDialog * self){
    GtkCssProvider * cssProvider;
    GtkStyleContext * context;
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    priv->panel_decor = gtk_grid_new ();
    //Add title strip
    priv->title_lbl = gtk_label_new("");
    gtk_widget_set_margin_bottom(priv->title_lbl,10);
    gtk_widget_set_hexpand (priv->title_lbl, TRUE);
    gtk_grid_attach (GTK_GRID (priv->panel_decor), priv->title_lbl, 0, 0, 1, 1);

    //Lightgrey background for the title strip
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; border-radius: 10px; padding: 10px; background: linear-gradient(to top, @theme_bg_color, @theme_bg_color);}",-1,NULL); 
    context = gtk_widget_get_style_context(priv->panel_decor);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    OnvifMgrAppDialog__create_buttons(self);
    gtk_grid_attach (GTK_GRID (priv->panel_decor), priv->action_panel, 0, 3, 1, 1);

    priv->lblerr = gtk_label_new("");
    gtk_widget_set_hexpand (priv->lblerr, TRUE);
    gtk_widget_set_vexpand (priv->lblerr, FALSE);
    gtk_widget_set_margin_bottom(priv->lblerr,15);
    gtk_widget_set_margin_top(priv->lblerr,5);
    gtk_grid_attach (GTK_GRID (priv->panel_decor), priv->lblerr, 0, 1, 2, 1);
    gtk_label_set_justify(GTK_LABEL(priv->lblerr), GTK_JUSTIFY_CENTER);
    gtk_widget_set_no_show_all(priv->lblerr,TRUE);

    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; color:#DE5E09;}",-1,NULL); 
    context = gtk_widget_get_style_context(priv->lblerr);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref (cssProvider);  

    return priv->panel_decor;
}

static void
OnvifMgrAppDialog__real_create_ui(OnvifMgrAppDialog * self){
    GtkWidget * empty;
    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    //See-through overlay background
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black; opacity:0.3;}",-1,NULL); 

    //Remove this to align left
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self), empty, 0, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align rights
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self), empty, 2, 0, 1, 3);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align top
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self), empty, 1, 0, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);

    //Remove this to align bot
    empty = gtk_label_new("");
    gtk_widget_set_vexpand (empty, TRUE);
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (self), empty, 1, 2, 1, 1);
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    empty = OnvifMgrAppDialog__create_panel(self);
    gtk_grid_attach (GTK_GRID (self), empty, 1, 1, 1, 1);

    //Creating empty panel, to create opacity background behind corners
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black; opacity:0.3;}",-1,NULL); 
    empty = gtk_label_new("");
    context = gtk_widget_get_style_context(empty);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_grid_attach (GTK_GRID (self), empty, 1, 1, 1, 1);

    g_object_unref (cssProvider);  
}

static void
OnvifMgrAppDialog__set_nofocus(GtkWidget *widget, GtkWidget * dialog_root){
    if(widget != dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

static void
OnvifMgrAppDialog__set_onlyfocus (GtkWidget *widget, GtkWidget * dialog_root){
    if(widget == dialog_root){
        gtk_widget_set_can_focus(widget,FALSE);
    } else {
        gtk_widget_set_can_focus(widget,TRUE);
    }
}

static void
OnvifMgrAppDialog__dispose (GObject *object){
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (ONVIFMGR_APPDIALOG(object));
    if(priv->focusedWidget && GTK_IS_WIDGET(priv->focusedWidget)){ //Call only if self destroys
        gtk_widget_grab_focus(priv->focusedWidget);
        priv->focusedWidget = NULL;
    }
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(object));
    if(parent && GTK_IS_CONTAINER(parent)){
        gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)OnvifMgrAppDialog__set_nofocus, object);
    }

    if(priv->title){
        free(priv->title);
        priv->title = NULL;
    }

    GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET(object));
    if (priv->keysignalid && gtk_widget_is_toplevel (window)){  
        g_signal_handler_disconnect (G_OBJECT (window), priv->keysignalid);
        priv->keysignalid = 0;
    }
    G_OBJECT_CLASS (OnvifMgrAppDialog__parent_class)->dispose (object);
}

static void
OnvifMgrAppDialog__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrAppDialog * self = ONVIFMGR_APPDIALOG (object);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    const char * strvalue;
    gboolean boolvalue;

    switch (prop_id){
        case PROP_TITLELABEL:
            strvalue = g_value_get_string (value);
            priv->title = malloc(strlen(strvalue)+1);
            strcpy(priv->title,strvalue);

            char * new_title = malloc(strlen(ONVIFMGR_APPDIALOG_TITLE_PREFIX) + strlen(priv->title) + strlen(ONVIFMGR_APPDIALOG_TITLE_SUFFIX) +1);
            strcpy(new_title,ONVIFMGR_APPDIALOG_TITLE_PREFIX);
            strcat(new_title,priv->title);
            strcat(new_title,ONVIFMGR_APPDIALOG_TITLE_SUFFIX);

            gtk_label_set_markup (GTK_LABEL (priv->title_lbl), new_title);
            free(new_title);
            break;
        case PROP_SUBMITLABEL:
            strvalue = g_value_get_string (value);
            if(GTK_IS_BUTTON(priv->submit_btn))
                gtk_button_set_label(GTK_BUTTON(priv->submit_btn), strvalue);
            break;
        case PROP_USERDATA:
            priv->user_data = g_value_get_pointer (value);
            break;
        case PROP_ACTIONVISIBLE:
            gtk_widget_set_visible(priv->action_panel,g_value_get_boolean (value));
            break;
        case PROP_CLOSABLE:
            priv->closable = g_value_get_boolean(value);
            break;
        case PROP_CANCELLABLE:
            boolvalue = g_value_get_boolean(value);
            if(GTK_IS_WIDGET(priv->cancel_btn) && !boolvalue){
                gtk_widget_destroy(priv->cancel_btn);
                priv->cancel_btn = NULL;
            } else if(!GTK_IS_WIDGET(priv->cancel_btn) && boolvalue){
                OnvifMgrAppDialog__attach_cancel_button(self);
            }
            break;
        case PROP_SUBMITTABLE:
            boolvalue = g_value_get_boolean(value);
            if(GTK_IS_WIDGET(priv->submit_btn) && !boolvalue){
                gtk_widget_destroy(priv->submit_btn);
                priv->submit_btn = NULL;
            } else if(!GTK_IS_WIDGET(priv->submit_btn) && boolvalue){
                OnvifMgrAppDialog__attach_submit_button(self);
            }
            break;
        case PROP_ERROR:
            strvalue = g_value_get_string(value);
            gtk_label_set_label(GTK_LABEL(priv->lblerr),strvalue);
            gtk_widget_set_visible(priv->lblerr,strvalue && strlen(strvalue));
            break;
        case PROP_SELFDESTROY:
            boolvalue = g_value_get_boolean(value);
            priv->self_destroy = boolvalue;
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrAppDialog__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifMgrAppDialog *thread = ONVIFMGR_APPDIALOG (object);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (thread);
    switch (prop_id){
        case PROP_TITLELABEL:
            g_value_set_string (value, priv->title);
            break;
        case PROP_SUBMITLABEL:
            g_value_set_string (value, gtk_button_get_label(GTK_BUTTON(priv->submit_btn)));
            break;
        case PROP_USERDATA:
            g_value_set_pointer (value, priv->user_data);
            break;
        case PROP_ACTIONVISIBLE:
            g_value_set_boolean(value,gtk_widget_is_visible(priv->action_panel));
            break;
        case PROP_CLOSABLE:
            g_value_set_boolean(value,priv->closable);
            break;
        case PROP_CANCELLABLE:
            g_value_set_boolean(value,GTK_IS_WIDGET(priv->cancel_btn));
            break;
        case PROP_SUBMITTABLE:
            g_value_set_boolean(value,GTK_IS_WIDGET(priv->submit_btn));
            break;
        case PROP_ERROR:
            g_value_set_string(value,gtk_label_get_label(GTK_LABEL(priv->lblerr)));
            break;
        case PROP_SELFDESTROY:
            g_value_set_boolean(value,priv->self_destroy);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrAppDialog__constructed (GObject *object){
    OnvifMgrAppDialog * self = ONVIFMGR_APPDIALOG(object);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    OnvifMgrAppDialogClass *klass = ONVIFMGR_APPDIALOG_GET_CLASS (self);
    g_return_if_fail (klass->create_ui != NULL);
    priv->panel = klass->create_ui (self);
    g_return_if_fail (priv->panel != NULL);
    gtk_grid_attach (GTK_GRID (priv->panel_decor), priv->panel, 0, 2, 1, 1);
    G_OBJECT_CLASS (OnvifMgrAppDialog__parent_class)->constructed (object);
}

// static int 
// OnvifMgrAppDialog__has_focus(GtkWidget * widget){
//     if(gtk_widget_has_focus(widget)){
//         return 1;
//     }

//     if (!GTK_IS_CONTAINER(widget)){
//         return 0;
//     }

//     GList * children = gtk_container_get_children(GTK_CONTAINER(widget));
//     GList * tmp = children;
//     while (tmp != NULL){
//         GtkWidget * child = tmp->data;
//         if(OnvifMgrAppDialog__has_focus(child)){
//             g_list_free (children);
//             return 1;
//         }
//         tmp = tmp->next;
//     }
//     g_list_free (children);
//     return 0;
// }

static gboolean 
OnvifMgrAppDialog__keypress_function (GtkWidget *widget, GdkEventKey *event, OnvifMgrAppDialog * self) {
    // if(!OnvifMgrAppDialog__has_focus(GTK_WIDGET(self))){
    //     return FALSE;
    // }

    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    if (event->keyval == GDK_KEY_Escape){
        OnvifMgrAppDialog__cancel_event(widget,self);
        return TRUE;
    } else if (gtk_widget_is_visible(priv->action_panel) &&
        (event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return) 
        && !gtk_widget_has_focus(priv->cancel_btn)
        && !gtk_widget_has_focus(priv->submit_btn)){
        OnvifMgrAppDialog__submit_event(widget,self);
        return TRUE;
    }
    return FALSE;
}

static void
OnvifMgrAppDialog__set_focus_child (GtkContainer * container, GtkWidget	 *child){
    GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET(container));
    if (!gtk_widget_is_visible(GTK_WIDGET(container)) && gtk_widget_is_toplevel (window)){
        OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (ONVIFMGR_APPDIALOG(container));
        priv->focusedWidget = gtk_window_get_focus(GTK_WINDOW(window));
    }

    GTK_CONTAINER_CLASS (OnvifMgrAppDialog__parent_class)->set_focus_child (container, child);
}

static void
OnvifMgrAppDialog__show (GtkWidget * widget){
    int visible_before = gtk_widget_is_visible(widget);
    GTK_WIDGET_CLASS (OnvifMgrAppDialog__parent_class)->show (widget);
    if(visible_before != gtk_widget_is_visible(widget)){
        OnvifMgrAppDialog *self = ONVIFMGR_APPDIALOG(widget);
        OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
        //Conncet keyboard handler. ESC = Cancel, Enter/Return = Login
        GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET(self));
        if (gtk_widget_is_toplevel (window)){  
            gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
            priv->keysignalid = g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (OnvifMgrAppDialog__keypress_function), self);
        }

        //Prevent overlayed widget from getting focus
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(self));
        gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)OnvifMgrAppDialog__set_onlyfocus, self);

        OnvifMgrAppDialogClass *klass = ONVIFMGR_APPDIALOG_GET_CLASS (self);
        if(klass->showing) klass->showing(widget);
        g_signal_emit (self, signals[SIGNAL_SHOWING], 0);
    }
}

static void
OnvifMgrAppDialog__hide (GtkWidget * widget){
    OnvifMgrAppDialog *self = ONVIFMGR_APPDIALOG(widget);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    
    //Disconnect keyboard handler.
    GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET(self));
    if (priv->keysignalid && gtk_widget_is_toplevel (window)){  
        g_signal_handler_disconnect (G_OBJECT (window), priv->keysignalid);
        priv->keysignalid = 0;
    }

    //Restore focus capabilities to previous widget
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(self));
    gtk_container_foreach (GTK_CONTAINER (parent), (GtkCallback)OnvifMgrAppDialog__set_nofocus, self);

    //Restore stolen focus
    if(GTK_IS_WIDGET(priv->focusedWidget)){
        gtk_widget_grab_focus(priv->focusedWidget);
        priv->focusedWidget = NULL;
    }

    GTK_WIDGET_CLASS (OnvifMgrAppDialog__parent_class)->hide (widget);
}

static void
OnvifMgrAppDialog__class_init (OnvifMgrAppDialogClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->constructed = OnvifMgrAppDialog__constructed;
    object_class->dispose = OnvifMgrAppDialog__dispose;
    object_class->set_property = OnvifMgrAppDialog__set_property;
    object_class->get_property = OnvifMgrAppDialog__get_property;
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
    container_class->set_focus_child = OnvifMgrAppDialog__set_focus_child;
    GtkWidgetClass * widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->show = OnvifMgrAppDialog__show;
    widget_class->hide = OnvifMgrAppDialog__hide;
    klass->create_ui = OnvifMgrAppDialog__dummy_create_ui;

    signals[SIGNAL_SUBMIT] =
        g_signal_newv ("submit",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);

    signals[SIGNAL_CANCEL] =
        g_signal_newv ("cancel",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);

    signals[SIGNAL_SHOWING] =
        g_signal_newv ("showing",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);

    obj_properties[PROP_TITLELABEL] =
        g_param_spec_string ("title-label",
                            "Title String",
                            "Text shown on the title bar",
                            default_title_label,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_SUBMITLABEL] =
        g_param_spec_string ("submit-label",
                            "Submit button String",
                            "Text shown on the submit button",
                            default_submit_label,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_USERDATA] = 
        g_param_spec_pointer ("userdata", 
                            "Pointer to user provided data", 
                            "Pointer to user provided data used in callbacks",
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_ACTIONVISIBLE] =
        g_param_spec_boolean ("action-visible",
                            "Actionbar visibility",
                            "gboolean flag to set action bar visibility",
                            TRUE,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_CLOSABLE] =
        g_param_spec_boolean ("closable",
                            "Dialog will close on cancel",
                            "gboolean flag to set the dialog as closable",
                            TRUE,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_CANCELLABLE] =
        g_param_spec_boolean ("cancellable",
                            "Dialog is cancellable",
                            "gboolean flag that will show a cancel button in the action bar",
                            TRUE,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_SUBMITTABLE] =
        g_param_spec_boolean ("submittable",
                            "Dialog is submittable",
                            "gboolean flag that will show a cancel button in the actioRn bar",
                            TRUE,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_SELFDESTROY] =
        g_param_spec_boolean ("self-destroy",
                            "Dialog will self destroy",
                            "The dialog will destroy itself instead of simply hiding",
                            TRUE,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_ERROR] =
        g_param_spec_string ("error",
                            "Dialog error string",
                            "Error message to display on dialog",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    
    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifMgrAppDialog__init (OnvifMgrAppDialog *self){
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    priv->closable = TRUE;
    priv->title = NULL;
    priv->panel = NULL;
    priv->panel_decor = NULL;
    priv->loading_panel = NULL;
    priv->submit_btn = NULL;
    priv->cancel_btn = NULL;
    priv->title_lbl = NULL;
    priv->action_panel = NULL;
    priv->innerFocusedWidget = NULL;

    OnvifMgrAppDialog__real_create_ui(self);
}

void 
OnvifMgrAppDialog__show_loading(OnvifMgrAppDialog * self, char * message){
    g_return_if_fail(self != NULL);
    g_return_if_fail(ONVIFMGR_IS_APPDIALOG(self));
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    //Save previous focused element
    GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET(self));
    if (gtk_widget_is_toplevel (window)){  
        priv->innerFocusedWidget = gtk_window_get_focus(GTK_WINDOW(window));
    }

    if(GTK_IS_WIDGET(priv->loading_panel)) gtk_widget_destroy(priv->loading_panel);

    //Create loading panel
    GtkWidget * widget;
    priv->loading_panel = gtk_grid_new ();
    gtk_widget_set_margin_bottom(priv->loading_panel,10);
    gtk_widget_set_margin_top(priv->loading_panel,10);
    gtk_widget_set_margin_start(priv->loading_panel,10);
    gtk_widget_set_margin_end(priv->loading_panel,10);  

    widget = gtk_label_new(message);
    gtk_widget_set_margin_bottom(widget,10);
    gtk_widget_set_margin_top(widget,10);
    gtk_widget_set_margin_start(widget,10);
    gtk_widget_set_margin_end(widget,10);  
    gtk_label_set_line_wrap(GTK_LABEL(widget),1);
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_grid_attach (GTK_GRID (priv->loading_panel), widget, 0, 1, 3, 1);
    gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_CENTER);

    widget = gtk_label_new("");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (priv->loading_panel), widget, 0, 0, 1, 1);

    widget = gtk_dotted_slider_new(GTK_ORIENTATION_HORIZONTAL, 5,10,1);
    gtk_widget_set_margin_bottom(widget,10);
    gtk_widget_set_margin_top(widget,10);
    gtk_widget_set_margin_start(widget,10);
    gtk_widget_set_margin_end(widget,10);  
    gtk_grid_attach (GTK_GRID (priv->loading_panel), widget, 1, 0, 1, 1);

    widget = gtk_label_new("");
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (priv->loading_panel), widget, 2, 0, 1, 1);

    //Hide input panel
    if(GTK_IS_WIDGET(priv->panel))
        gtk_widget_set_visible(priv->panel,FALSE);

    //Hide action panel
    if(GTK_IS_WIDGET(priv->action_panel))
        gtk_widget_set_visible(priv->action_panel,FALSE);

    //Hide error panel
    if(GTK_IS_WIDGET(priv->lblerr))
        gtk_widget_set_visible(priv->lblerr,FALSE);

    //Attach and show new panel
    gtk_grid_attach (GTK_GRID (priv->panel_decor), priv->loading_panel, 0, 1, 1, 1);
    gtk_widget_show_all(priv->loading_panel);
}

void 
OnvifMgrAppDialog__hide_loading(OnvifMgrAppDialog * self){
    g_return_if_fail(self != NULL);
    g_return_if_fail(ONVIFMGR_IS_APPDIALOG(self));
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);

    //Destroy loading panel
    if(GTK_IS_WIDGET(priv->loading_panel)){
        gtk_widget_destroy(priv->loading_panel);
        priv->loading_panel = NULL;
    }
    
    //Show input panel
    if(GTK_IS_WIDGET(priv->panel))
        gtk_widget_show(priv->panel);

    //Show action panel
    if(GTK_IS_WIDGET(priv->action_panel))
        gtk_widget_show(priv->action_panel);

    //Show error panel
    if(GTK_IS_WIDGET(priv->lblerr) && strlen(gtk_label_get_label(GTK_LABEL(priv->lblerr))))
        gtk_widget_show(priv->lblerr);

    //Restore stolen focus
    if(GTK_IS_WIDGET(priv->innerFocusedWidget)){
        gtk_widget_grab_focus(priv->innerFocusedWidget);
        priv->innerFocusedWidget = NULL;
    }
}

gboolean OnvifMgrAppDialog__close(OnvifMgrAppDialog * self){
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(ONVIFMGR_IS_APPDIALOG(self), FALSE);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    if(priv->self_destroy){
        gtk_widget_destroy(GTK_WIDGET(self));
    } else {
        gtk_widget_hide(GTK_WIDGET(self));
    }

    return FALSE;
}

void OnvifMgrAppDialog__add_action_widget(OnvifMgrAppDialog * self, GtkWidget * widget, OnvifMgrAppDialogButtonPosition position){
    g_return_if_fail(self != NULL);
    g_return_if_fail(ONVIFMGR_IS_APPDIALOG(self));
    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_WIDGET(widget));
    g_return_if_fail(position >= ONVIFMGR_APPDIALOG_BUTTON_BEFORE && position <= ONVIFMGR_APPDIALOG_BUTTON_AFTER);
    OnvifMgrAppDialogPrivate *priv = OnvifMgrAppDialog__get_instance_private (self);
    gtk_grid_attach (GTK_GRID (priv->action_panel), widget, position, 0, 1, 1);

}

OnvifMgrAppDialog * 
OnvifMgrAppDialog__new(){
    return g_object_new (ONVIFMGR_TYPE_APPDIALOG,
                        NULL);
}