#include "onvif_details_panel.h"
#include "../gui_utils.h"
#include "clogger.h"

enum {
    STARTED,
    FINISHED,
    LAST_SIGNAL
};

enum {
  PROP_APP = 1,
  N_PROPERTIES
};

typedef struct {
    OnvifApp * app;
    OnvifMgrDeviceRow * device;

    QueueEvent * previous_event;
    gulong event_signal;
} OnvifDetailsPanelPrivate;

typedef struct {
    OnvifDetailsPanel * panel;
    OnvifMgrDeviceRow * device;
    void * data;
    QueueEvent * qevt; //Keep reference to check iscancelled before rendering result
} OnvifDetailsPanelUpdateRequest;


static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(OnvifDetailsPanel, OnvifDetailsPanel_, GTK_TYPE_SCROLLED_WINDOW)

static void 
OnvifDetailsPanel__update_details_page_cleanup(QueueEvent * qevt, int cancelled, void * user_data){
    OnvifDetailsPanelUpdateRequest * request = (OnvifDetailsPanelUpdateRequest *) user_data;
    g_object_unref(request->device);
    free(request);
}

static void 
OnvifDetailsPanel_update_details_event_cancelled_cb(QueueEvent * qevt, QueueEventState state, void * user_data){
    OnvifDetailsPanelUpdateRequest * request = (OnvifDetailsPanelUpdateRequest *) user_data;
    //Do not dispatch on QUEUEEVENT_DISPATCHED.
    //FINISHED will be invoked at the end of the GUI update
    if(state == QUEUEEVENT_CANCELLED)
        gui_signal_emit(request->panel, signals[FINISHED], request->device);
}

static gboolean 
OnvifDetailsPanel_gui_update (void * user_data){
    OnvifDetailsPanelUpdateRequest * request = (OnvifDetailsPanelUpdateRequest *) user_data;
    OnvifDetailsPanelClass *klass = ONVIFMGR_DETAILSPANEL_GET_CLASS (request->panel);
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (request->panel);

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(request->device) || !OnvifMgrDeviceRow__is_selected(request->device) || QueueEvent__is_cancelled(request->qevt)){
        goto exit;
    }

    if(klass->updateui)
        klass->updateui (request->panel,priv->app,request->device, request->data);
    else
        C_ERROR("updateui virtual method unset.");

    g_signal_emit (request->panel, signals[FINISHED], 0, request->device);
exit:
    g_object_unref(request->device);
    g_object_unref(request->qevt);
    free(request);

    return FALSE;
}

static void 
OnvifDetailsPanel__update_details_page(QueueEvent * qevt, void * user_data){
    OnvifDetailsPanelUpdateRequest * request = (OnvifDetailsPanelUpdateRequest *) user_data;
    OnvifDetailsPanelClass *klass = ONVIFMGR_DETAILSPANEL_GET_CLASS (request->panel);
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (request->panel);
    request->qevt = qevt;

    g_return_if_fail (klass->getdata != NULL);
    request->data = klass->getdata (request->panel,priv->app,request->device, request->qevt);

    if(!request->data){
        free(request);
        return;
    }

    //Make a copy so that the Queue event can safely cleanup device reference even if the GUI dispatched first
    OnvifDetailsPanelUpdateRequest * ui_request = malloc(sizeof(OnvifDetailsPanelUpdateRequest));
    memcpy (ui_request, request, sizeof (OnvifDetailsPanelUpdateRequest));

    g_object_ref(ui_request->device);
    g_object_ref(ui_request->qevt);
    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifDetailsPanel_gui_update),ui_request);
}

static void 
OnvifDetailsPanel_update_details(OnvifDetailsPanel * self){
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (self);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(priv->device)){
        return;
    }
    
    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(priv->device);
    if(!OnvifDevice__is_authenticated(odev)){
        return;
    }

    OnvifDetailsPanelUpdateRequest * request = malloc(sizeof(OnvifDetailsPanelUpdateRequest));
    request->device = priv->device;
    request->panel = self;
    g_object_ref(request->device);
    if(priv->previous_event && !QueueEvent__is_finished(priv->previous_event)) {
        g_signal_handler_disconnect(priv->previous_event,priv->event_signal); //Removing signal to prevent loading from hiding
        QueueEvent__cancel(priv->previous_event);
        g_object_unref(priv->previous_event);
        priv->previous_event = NULL;
        priv->event_signal = 0;
    } else if(priv->previous_event){
        g_object_unref(priv->previous_event);
    }

    g_signal_emit (self, signals[STARTED], 0, request->device);
    priv->previous_event = EventQueue__insert_plain(OnvifApp__get_EventQueue(priv->app), request->device, OnvifDetailsPanel__update_details_page,request, OnvifDetailsPanel__update_details_page_cleanup);
    priv->event_signal = g_signal_connect (priv->previous_event, "state-changed", G_CALLBACK (OnvifDetailsPanel_update_details_event_cancelled_cb), request);
    g_object_ref(priv->previous_event);
}

static void 
OnvifDetailsPanel__device_changed_cb(OnvifApp * app, OnvifMgrDeviceRow * device, OnvifDetailsPanel * panel){
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (panel);
    OnvifDetailsPanelClass *klass = ONVIFMGR_DETAILSPANEL_GET_CLASS (panel);
    priv->device = device;

    g_return_if_fail (klass->clearui != NULL);
    klass->clearui (panel);
    if(gtk_widget_get_mapped(GTK_WIDGET(panel)) && ONVIFMGR_IS_DEVICEROW(device) && OnvifMgrDeviceRow__is_initialized(device)){
        OnvifDetailsPanel_update_details(panel);
    }
}

static void 
OnvifDetailsPanel__map_event_cb (GtkWidget* self){
    OnvifDetailsPanel * panel = ONVIFMGR_DETAILSPANEL(self);
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (panel);
    if(gtk_widget_get_mapped(self) && ONVIFMGR_IS_DEVICEROW(priv->device) && OnvifMgrDeviceRow__is_initialized(priv->device)){
        OnvifDetailsPanelClass *klass = ONVIFMGR_DETAILSPANEL_GET_CLASS (panel);
        g_return_if_fail (klass->clearui != NULL);
        klass->clearui (panel);
        OnvifDetailsPanel_update_details(panel);
    }
}

static void
OnvifDetailsPanel__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifDetailsPanel * info = ONVIFMGR_DETAILSPANEL (object);
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (info);
    switch (prop_id){
        case PROP_APP:
            priv->app = ONVIFMGR_APP(g_value_get_object (value));
            g_signal_connect (priv->app, "device-changed", G_CALLBACK (OnvifDetailsPanel__device_changed_cb), info);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifDetailsPanel__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifDetailsPanel *info = ONVIFMGR_DETAILSPANEL (object);
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (info);
    switch (prop_id){
        case PROP_APP:
            g_value_set_object (value, priv->app);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void 
OnvifDetailsPanel__constructed(GObject* object){
    OnvifDetailsPanelClass *klass = ONVIFMGR_DETAILSPANEL_GET_CLASS (object);
    if(klass->createui != NULL)
        klass->createui (ONVIFMGR_DETAILSPANEL(object));
    else
        C_INFO("No createui virtualmethod");

    G_OBJECT_CLASS (OnvifDetailsPanel__parent_class)->constructed (object);
}


static void
OnvifDetailsPanel__class_init (OnvifDetailsPanelClass * klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->constructed = OnvifDetailsPanel__constructed;
    object_class->set_property = OnvifDetailsPanel__set_property;
    object_class->get_property = OnvifDetailsPanel__get_property;

    obj_properties[PROP_APP] =
        g_param_spec_object ("app",
                            "OnvifApp",
                            "Pointer to OnvifApp parent.",
                            ONVIFMGR_TYPE_APP  /* default value */,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    GType params[1];
    params[0] = ONVIFMGR_TYPE_DEVICEROW | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[STARTED] =
        g_signal_newv ("started",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);

    signals[FINISHED] =
        g_signal_newv ("finished",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);
                        
    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifDetailsPanel__init (OnvifDetailsPanel * self){
    OnvifDetailsPanelPrivate *priv = OnvifDetailsPanel__get_instance_private (self);
    priv->app = NULL;
    priv->device = NULL;

    g_signal_connect (self, "map", G_CALLBACK (OnvifDetailsPanel__map_event_cb), NULL);
}