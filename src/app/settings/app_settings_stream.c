#include "app_settings_stream.h"

#define APPSETTINGS_STREAM_CAT "stream"

enum {
  SETTINGS_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_VIEW_MODE = 1,
  N_PROPERTIES
};

typedef struct {
    GtkWidget * view_mode_box;
    int view_mode;
} AppSettingsStreamPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(AppSettingsStream, AppSettingsStream_, GTK_TYPE_GRID)

static void
AppSettingsStream__get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  AppSettingsStream *self = APPSETTINGS_STREAM (object);

  switch (property_id)
   {
    case PROP_VIEW_MODE:
      g_value_set_int (value, AppSettingsStream__get_view_mode (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
AppSettingsStream__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    AppSettingsStream * info = APPSETTINGS_STREAM (object);
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (info);
    switch (prop_id){
        case PROP_VIEW_MODE:
            priv->view_mode = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
AppSettingsStream__class_init (AppSettingsStreamClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->get_property = AppSettingsStream__get_property;
    object_class->set_property = AppSettingsStream__set_property;

    signals[SETTINGS_CHANGED] =
        g_signal_newv ("settings-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);

    obj_properties[PROP_VIEW_MODE] =
        g_param_spec_int ("view-mode",
                            "View Mode",
                            "View Mode setting value.",
                            0, G_MAXINT, 0,  /* default value */
                            G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

//Generic value callback for togglebuttons
void value_toggled (GtkToggleButton* self, AppSettingsStream * settings){
    //On value changed, send signal to check to notify parent panel
    g_signal_emit (settings, signals[SETTINGS_CHANGED], 0 /* details */);
}

void AppSettingsStream__create_ui(AppSettingsStream * self){
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));
    //Add stream page properties
    g_object_set (self, "margin", 20, NULL);
    gtk_widget_set_hexpand (GTK_WIDGET(self), TRUE);
    priv->view_mode_box = gtk_check_button_new_with_label("Allow overscaling");
    gtk_grid_attach (GTK_GRID (self), priv->view_mode_box, 0, 1, 1, 1);

    g_signal_connect (G_OBJECT (priv->view_mode_box), "toggled", G_CALLBACK (value_toggled), self);
}

static void
AppSettingsStream__init (AppSettingsStream * self)
{
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));
    priv->view_mode = 0;
    AppSettingsStream__create_ui(self);
}

AppSettingsStream * AppSettingsStream__new(){
    return g_object_new (APPSETTINGS_TYPE_STREAM, NULL);
}

int AppSettingsStream__get_state (AppSettingsStream * self){
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));
    int scale_val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->view_mode_box));

    //More settings widgets here
    return scale_val != priv->view_mode;
}

void AppSettingsStream__set_state(AppSettingsStream * self,int state){
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));
    if(GTK_IS_WIDGET(priv->view_mode_box))
        gtk_widget_set_sensitive(priv->view_mode_box,state);
}

int AppSettingsStream__get_view_mode(AppSettingsStream * self){
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));
    return priv->view_mode;
}

char * AppSettingsStream__save(AppSettingsStream * self){
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));

    if(AppSettingsStream__get_state(self)){
        int newval = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->view_mode_box));
        if(newval != priv->view_mode){
            priv->view_mode = newval;
            g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_VIEW_MODE]);
        } 
    }
    if(priv->view_mode){
        return "[stream]\nview-mode=1";
    } else {
        return "[stream]\nview-mode=0";
    }
}

void AppSettingsStream__reset(AppSettingsStream * self){
    AppSettingsStreamPrivate *priv = AppSettingsStream__get_instance_private (APPSETTINGS_STREAM(self));
    if(priv->view_mode){
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->view_mode_box),TRUE);
    } else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->view_mode_box),FALSE);
    }
}

char * AppSettingsStream__get_category(AppSettingsStream * self){
    return APPSETTINGS_STREAM_CAT;
}