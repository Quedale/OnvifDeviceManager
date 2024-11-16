#include "omgr_profiles_dialog.h"
#include "clogger.h"
#include "../../animations/gtk/gtk_dotted_slider_widget.h"
#include "gtkprofilepanel.h"

enum {
    SIGNAL_SELECTED,
    LAST_SIGNAL
};

enum
{
  PROP_QUEUE = 1,
  PROP_DEVICE = 2,
  N_PROPERTIES
};

typedef struct {
  GtkWidget * primary_pane;
  GtkWidget * content_pane;

  OnvifMediaProfiles * profiles;
  OnvifMgrDeviceRow * device;
  EventQueue * queue;
} OnvifMgrProfilesDialogPrivate;

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrProfilesDialog, OnvifMgrProfilesDialog_, ONVIFMGR_TYPE_APPDIALOG)

static GtkWidget * 
OnvifMgrProfilesDialog__create_ui(OnvifMgrAppDialog * app_dialog){
  OnvifMgrProfilesDialogPrivate *priv = OnvifMgrProfilesDialog__get_instance_private (ONVIFMGR_PROFILESDIALOG(app_dialog));
  priv->primary_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  g_object_set (app_dialog,
          "title-label", "Profile Selector",
          "submittable", FALSE,
          NULL);

  return priv->primary_pane;
}

static void
OnvifMgrProfilesDialog__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrProfilesDialog * self = ONVIFMGR_PROFILESDIALOG (object);
    OnvifMgrProfilesDialogPrivate *priv = OnvifMgrProfilesDialog__get_instance_private (self);

    switch (prop_id){
        case PROP_QUEUE:
            priv->queue = g_value_get_object (value);
            break;
        case PROP_DEVICE:
            priv->device = g_value_get_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrProfilesDialog__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifMgrProfilesDialog * self = ONVIFMGR_PROFILESDIALOG (object);
    OnvifMgrProfilesDialogPrivate *priv = OnvifMgrProfilesDialog__get_instance_private (self);
    switch (prop_id){
        case PROP_QUEUE:
            g_value_set_object (value, priv->queue);
            break;
        case PROP_DEVICE:
            g_value_set_object (value, priv->device);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void 
OnvifMgrProfilesDialog__profile_clicked (GtkProfilePanel * button, OnvifMgrProfilesDialog * dialog) {
  g_signal_emit (dialog, signals[SIGNAL_SELECTED], 0, gtk_profile_panel_get_profile(button));
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean 
OnvifMgrProfilesDialog__show_profiles(void * user_data){
  OnvifMgrProfilesDialog * profile_dialog = ONVIFMGR_PROFILESDIALOG(user_data);
  OnvifMgrProfilesDialogPrivate *priv = OnvifMgrProfilesDialog__get_instance_private (profile_dialog);
  OnvifMgrAppDialog__hide_loading(ONVIFMGR_APPDIALOG(profile_dialog));
  //TODO Check fault
  if(priv->content_pane && GTK_IS_WIDGET(priv->content_pane)) gtk_widget_destroy(priv->content_pane);

  priv->content_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  for(int i=0;i<OnvifMediaProfiles__get_size(priv->profiles);i++){
      OnvifProfile * profile = OnvifMediaProfiles__get_profile(priv->profiles,i);

      GtkWidget * btn = gtk_profile_panel_new(profile);
      gtk_box_pack_start(GTK_BOX(priv->content_pane), btn,     FALSE, FALSE, 0);
      g_signal_connect (G_OBJECT(btn), "clicked", G_CALLBACK (OnvifMgrProfilesDialog__profile_clicked), profile_dialog);
  }

  gtk_box_pack_start(GTK_BOX(priv->primary_pane), priv->content_pane,     FALSE, FALSE, 0);
  gtk_widget_show_all (priv->primary_pane);
  
  g_object_unref(priv->profiles);
  priv->profiles = NULL;
  return FALSE;
}

static void 
OnvifMgrProfilesDialog__load_profiles(QueueEvent * qevt, void * user_data){
  OnvifMgrProfilesDialog* profile_dialog = ONVIFMGR_PROFILESDIALOG(user_data);
  OnvifMgrProfilesDialogPrivate *priv = OnvifMgrProfilesDialog__get_instance_private (profile_dialog);
  OnvifMediaService * mserv = OnvifDevice__get_media_service(OnvifMgrDeviceRow__get_device(priv->device));
  priv->profiles = OnvifMediaService__get_profiles(mserv);
  //TODO Error handling for profiles fault
  gdk_threads_add_idle(G_SOURCE_FUNC(OnvifMgrProfilesDialog__show_profiles),profile_dialog);
}

static void
OnvifMgrProfilesDialog__showing (GtkWidget *widget){
  OnvifMgrProfilesDialog * self = ONVIFMGR_PROFILESDIALOG(widget);
  OnvifMgrProfilesDialogPrivate *priv = OnvifMgrProfilesDialog__get_instance_private (self);
  OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(self),"Loading device profiles...");
  EventQueue__insert_plain(priv->queue, priv->device, OnvifMgrProfilesDialog__load_profiles,self, NULL);
}

static void
OnvifMgrProfilesDialog__class_init (OnvifMgrProfilesDialogClass *klass){
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->set_property = OnvifMgrProfilesDialog__set_property;
  object_class->get_property = OnvifMgrProfilesDialog__get_property;
  OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
  app_class->create_ui = OnvifMgrProfilesDialog__create_ui;
  app_class->showing = OnvifMgrProfilesDialog__showing;

  GType params[1];
  params[0] = G_TYPE_POINTER;
  signals[SIGNAL_SELECTED] =
      g_signal_newv ("profile-selected",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                      NULL /* closure */,
                      NULL /* accumulator */,
                      NULL /* accumulator data */,
                      NULL /* C marshaller */,
                      G_TYPE_NONE /* return_type */,
                      1     /* n_params */,
                      params  /* param_types */);

  obj_properties[PROP_QUEUE] =
      g_param_spec_object ("queue",
                          "EventQueue",
                          "Pointer to EventQueue parent.",
                          QUEUE_TYPE_EVENTQUEUE,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  obj_properties[PROP_DEVICE] =
      g_param_spec_object ("device",
                          "OnvifMgrDeviceRow",
                          "Device to display profiles.",
                          ONVIFMGR_TYPE_DEVICEROW,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                      N_PROPERTIES,
                                      obj_properties);
}

static void
OnvifMgrProfilesDialog__init (OnvifMgrProfilesDialog *self){

}

OnvifMgrProfilesDialog * 
OnvifMgrProfilesDialog__new(EventQueue * queue, OnvifMgrDeviceRow * device){
  return g_object_new (ONVIFMGR_TYPE_PROFILESDIALOG,
                      "queue",queue,
                      "device",device,
                      NULL);
}