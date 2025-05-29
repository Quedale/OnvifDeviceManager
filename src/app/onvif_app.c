#include "onvif_app.h"
#include "../queue/event_queue.h"
#include "dialog/omgr_add_dialog.h"
#include "dialog/omgr_credentials_dialog.h"
#include "dialog/omgr_profiles_dialog.h"
#include "dialog/omgr_encrypted_store.h"
#include "details/onvif_details.h"
#include "onvif_nvt.h"
#include "settings/app_settings.h"
#include "task_manager.h"
#include "clogger.h"
#include "gui_utils.h"
#include "gtkstyledimage.h"
#include "discoverer.h"
#include "onvif_app_shutdown.h"

extern char _binary_tower_png_size[];
extern char _binary_tower_png_start[];
extern char _binary_tower_png_end[];

enum {
    DEVICE_CHANGED,
    LAST_SIGNAL
};

typedef struct {
    OnvifMgrDeviceRow * device;

    int owned;

    GtkWidget *btn_scan;
    GtkWidget *window;
    GtkWidget *listbox;
    GtkWidget *player_loading_handle;
    GtkWidget *task_label;
    GtkOverlay * overlay;

    OnvifDetails * details;
    AppSettings * settings;

    EventQueue * queue;
    OnvifMgrEncryptedStore * store;
    GstRtspPlayer * player;
} OnvifAppPrivate;

struct IdleRetryData {
    GstRtspPlayer * player;
    GstRtspPlayerSession * session;
    OnvifMgrDeviceRow * device;
};

static guint signals[LAST_SIGNAL] = { 0 };

static void OnvifApp__ownable_interface_init (COwnableObjectInterface *iface);

G_DEFINE_TYPE_WITH_CODE(OnvifApp, OnvifApp_, G_TYPE_OBJECT, G_ADD_PRIVATE (OnvifApp)
                                    G_IMPLEMENT_INTERFACE (COWNABLE_TYPE_OBJECT,OnvifApp__ownable_interface_init))

static void OnvifApp__profile_changed_cb (OnvifMgrDeviceRow *device);
void OnvifApp__cred_dialog_login_cb(OnvifMgrAppDialog * app_dialog, OnvifMgrDeviceRow * device);
void OnvifApp__cred_dialog_cancel_cb(OnvifMgrAppDialog * app_dialog, OnvifMgrDeviceRow * device);
static gboolean OnvifApp__discovery_finished_cb (OnvifApp * self);
static gboolean OnvifApp__disocvery_found_server_cb (DiscoveryEvent * event);
static int OnvifApp__device_already_exist(OnvifApp * app, char * xaddr);
static void OnvifApp__add_device(OnvifApp * app, OnvifMgrDeviceRow * omgr_device);
static void OnvifApp__select_device(OnvifApp * app,  GtkListBoxRow * row);
static SoapFault OnvifApp__reload_device(QueueEvent * qevt, OnvifMgrDeviceRow * device);
static void OnvifApp__display_device(OnvifApp * self, OnvifMgrDeviceRow * device);

gboolean idle_select_device(void * user_data){
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);
    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) && gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(device))){
        OnvifApp__select_device(OnvifMgrDeviceRow__get_app(device),GTK_LIST_BOX_ROW(device));
    }
    g_object_unref(device);
    return FALSE;
}

gboolean idle_show_credentialsdialog (void * user_data){
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (OnvifMgrDeviceRow__get_app(device));
    OnvifMgrCredentialsDialog * dialog = OnvifMgrCredentialsDialog__new();
    g_object_set (dialog, "userdata", device, NULL);
    g_signal_connect (G_OBJECT (dialog), "submit", G_CALLBACK (OnvifApp__cred_dialog_login_cb), device);
    g_signal_connect (G_OBJECT (dialog), "cancel", G_CALLBACK (OnvifApp__cred_dialog_cancel_cb), device);
    gtk_overlay_add_overlay(priv->overlay,GTK_WIDGET(dialog));
    gtk_widget_show_all(GTK_WIDGET(priv->overlay));
    return FALSE;
}

gboolean idle_hide_dialog_loading (OnvifMgrAppDialog * app_dialog){
    OnvifMgrAppDialog__hide_loading(app_dialog);
    return FALSE;
}


gboolean idle_add_device(void * user_data){
    OnvifMgrDeviceRow * dev = ONVIFMGR_DEVICEROW(user_data);
    OnvifApp__add_device(OnvifMgrDeviceRow__get_app(dev),dev);
    return FALSE;
}

gboolean idle_player_retry_stream(void * user_data){
    struct IdleRetryData * data = (struct IdleRetryData *) user_data;
    //Check that the session to retry is still the one active
    if(GstRtspPlayer__get_session(data->player) == data->session){
        ONVIFMGR_DEVICEROW_TRACE("%s idle_player_retry_stream",data->device);
        GstRtspPlayerSession__retry(data->session);
    }
    g_object_unref(data->device);
    free(user_data);
    return FALSE;
}

void _wait_before_retry_stream(QueueEvent * qevt, void * user_data){
    struct IdleRetryData * data = (struct IdleRetryData *) user_data;
    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(data->device) && OnvifMgrDeviceRow__is_selected(data->device) && !QueueEvent__is_cancelled(qevt)){
        ONVIFMGR_DEVICEROW_TRACE("%s _wait_before_retry_stream",data->device);
        sleep(2);//Wait to avoid spamming the camera
        if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(data->device) && OnvifMgrDeviceRow__is_selected(data->device) && !QueueEvent__is_cancelled(qevt)){
            g_object_ref(data->device);
            g_idle_add(G_SOURCE_FUNC(idle_player_retry_stream), data);
            return;
        }
    }
    free(user_data);
}

void _dicovery_found_server_cb (DiscoveryEvent * event) {
    C_TRACE("_dicovery_found_server_cb");
    OnvifApp * app = (OnvifApp *) event->data;
    if(!COwnableObject__has_owner(COWNABLE_OBJECT(app))){
        return;
    }
    
    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifApp__disocvery_found_server_cb),event);
}

void _start_onvif_discovery(QueueEvent * qevt, void * user_data){
    C_TRACE("_start_onvif_discovery");
    OnvifApp * self = ONVIFMGR_APP(user_data);
    if(!COwnableObject__has_owner(COWNABLE_OBJECT(self))){
        return;
    }
    
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    //Start UDP Scan
    struct UdpDiscoverer discoverer = UdpDiscoverer__create(_dicovery_found_server_cb);
    UdpDiscoverer__start(&discoverer, self, AppSettingsDiscovery__get_repeat(priv->settings->discovery), AppSettingsDiscovery__get_timeout(priv->settings->discovery));
    //TODO Support discovery cancel? (No way to cancel it as of right now)
    g_object_ref(self);
    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifApp__discovery_finished_cb),self);
}

void _display_onvif_device(QueueEvent * qevt, void * user_data){
    C_TRACE("_display_onvif_device");
    OnvifMgrDeviceRow * omgr_device = (OnvifMgrDeviceRow *) user_data;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(omgr_device) || QueueEvent__is_cancelled(qevt)){
        return;
    }
    
    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(omgr_device);

    /* Authentication check */
    OnvifDevice__authenticate(odev);

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(omgr_device) || QueueEvent__is_cancelled(qevt)){
        return;
    } else if (!OnvifDevice__is_authenticated(odev)){
        goto updatethumb;
    }

    /* Display Profile dropdown */
    if(!OnvifMgrDeviceRow__get_profile(omgr_device)){
        OnvifMediaProfiles * profiles = OnvifMediaService__get_profiles(OnvifDevice__get_media_service(odev));
        //TODO error handling for profiles fault
        OnvifMgrDeviceRow__set_profile(omgr_device,OnvifMediaProfiles__get_profile(profiles,0));
        g_object_unref(profiles);
        //We don't care for the initial profile event since the default index is 0.
        //Connecting to signal only after setting the default profile
        g_signal_connect (G_OBJECT (omgr_device), "profile-changed", G_CALLBACK (OnvifApp__profile_changed_cb), NULL);
    }

    /* Display row thumbnail. Default to profile index 0 */
updatethumb:
    OnvifMgrDeviceRow__load_thumbnail(omgr_device);
    if(!OnvifMgrDeviceRow__is_initialized(omgr_device)){ //First time initialization
        OnvifMgrDeviceRow__set_initialized(omgr_device);
        //If it was selected while initializing, redispatch select_device
        if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(omgr_device) && OnvifMgrDeviceRow__is_selected(omgr_device) && !QueueEvent__is_cancelled(qevt)){
            g_object_ref(omgr_device);
            //TODO Handle event cancellation within GUI event
            gdk_threads_add_idle(G_SOURCE_FUNC(idle_select_device),omgr_device);
        }
    }
}

void _profile_callback (QueueEvent * qevt, void * user_data){
    C_TRACE("_profile_callback");
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)){
        C_TRAIL("_profile_callback - invalid device.");
        return;
    }
    
    if(OnvifMgrDeviceRow__is_selected(device) && !QueueEvent__is_cancelled(qevt)){
        OnvifAppPrivate *priv = OnvifApp__get_instance_private (OnvifMgrDeviceRow__get_app(device));
        GstRtspPlayer__stop(priv->player);
    }
    OnvifApp__reload_device(qevt, device);
    //TODO Show SoapFault error dialog

}

void _play_onvif_stream(QueueEvent * qevt, void * user_data){
    ONVIFMGR_DEVICEROW_TRACE("_play_onvif_stream %s",user_data);
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);

    //Check if device is still valid. (User performed scan before thread started)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device) || (qevt != NULL && QueueEvent__is_cancelled(qevt))){
        return;
    }

    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);
    OnvifApp * app = OnvifMgrDeviceRow__get_app(device);
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    
    /* Authentication check */
    OnvifDevice__authenticate(odev);

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || QueueEvent__is_cancelled(qevt) || !OnvifDevice__is_authenticated(odev) || !OnvifMgrDeviceRow__is_selected(device)) {
        return;
    }

    /* Set the URI to play */
    OnvifMediaProfile * profile = OnvifMgrDeviceRow__get_profile(device);
    OnvifUri * media_uri = OnvifMediaService__getStreamUri(OnvifDevice__get_media_service(odev),(profile) ? OnvifMediaProfile__get_index(profile) : 0);
    SoapFault * fault = SoapObject__get_fault(SOAP_OBJECT(media_uri));
    
    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) && *fault == SOAP_FAULT_NONE && !(qevt != NULL && QueueEvent__is_cancelled(qevt))){
        OnvifCredentials * ocreds = OnvifDevice__get_credentials(odev);
        char * user = OnvifCredentials__get_username(ocreds);
        char * pass = OnvifCredentials__get_password(ocreds);
        char * port = OnvifDevice__get_port(OnvifMgrDeviceRow__get_device(device));
        char * host = OnvifDevice__get_host(OnvifMgrDeviceRow__get_device(device));
        GstRtspPlayer__play(priv->player,OnvifUri__get_uri(media_uri),user,pass,host,port, device);
        if(pass)
            free(pass);
        if(user)
            free(user);
        if(port)
            free(port);
        if(host)
            free(host);

    } else if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)) {
        C_TRAIL("_play_onvif_stream - invalid device.");
    }
    g_object_unref(media_uri);
}

void _stop_onvif_stream(QueueEvent * qevt, void * user_data){
    C_TRACE("_stop_onvif_stream");
    OnvifApp * app = ONVIFMGR_APP(user_data);
    if(!COwnableObject__has_owner(COWNABLE_OBJECT(app))){
        return;
    }

    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    GstRtspPlayer__stop(priv->player);
}

void _onvif_authentication_reload_cleanup(QueueEvent * qevt, int cancelled, void * user_data){
    OnvifMgrDeviceRow * device = NULL;
    g_object_get (G_OBJECT (user_data), "userdata", &device, NULL);
    g_object_unref(device);
}

void _onvif_authentication_reload(QueueEvent * qevt, void * user_data){
    OnvifMgrCredentialsDialog * cred_dialog = ONVIFMGR_CREDENTIALSDIALOG(user_data);
    OnvifMgrDeviceRow * device = NULL;
    g_object_get (G_OBJECT (cred_dialog), "userdata", &device, NULL);

    C_DEBUG("_onvif_authentication_reload\n");
    //Check device is still valid before adding ref (User performed scan before thread started)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || QueueEvent__is_cancelled(qevt)){
        return;
    }

    SoapFault fault = OnvifApp__reload_device(qevt, device);
    switch(fault){
        case SOAP_FAULT_NONE:
            safely_destroy_widget(GTK_WIDGET(cred_dialog));
            g_object_unref(device);
            return;
        case SOAP_FAULT_CONNECTION_ERROR:
            g_object_set (cred_dialog, "error", "Failed to connect...", NULL);
            break;
        case SOAP_FAULT_NOT_VALID:
            g_object_set (cred_dialog, "error", "Not a valid ONVIF response...", NULL);
            break;
        case SOAP_FAULT_UNAUTHORIZED:
            g_object_set (cred_dialog, "error", "Unauthorized...", NULL);
            break;
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
            g_object_set (cred_dialog, "error", "Action Not Supported...", NULL);
            break;
        case SOAP_FAULT_UNEXPECTED:
            g_object_set (cred_dialog, "error", "Unexpected error...", NULL);
            break;
        default:
            g_object_set (cred_dialog, "error", "Unhandled error...", NULL);
            break;
    }
    gdk_threads_add_idle(G_SOURCE_FUNC(idle_hide_dialog_loading),cred_dialog);
}

void _onvif_device_add_cleanup(QueueEvent * qevt, int cancelled, void * user_data){
    free(user_data);
}

void _onvif_device_add(QueueEvent * qevt, void * user_data){
    OnvifMgrAddDialog * dialog = ONVIFMGR_ADDDIALOG(user_data);

    const char * host = OnvifMgrAddDialog__get_host(dialog);
    const char * port = OnvifMgrAddDialog__get_port(dialog);
    const char * protocol = OnvifMgrAddDialog__get_protocol(dialog);

    char fullurl[strlen(protocol) + 3 + strlen(host) + 1 + strlen(port) + strlen("/onvif/device_service") + 1];
    strcpy(fullurl,protocol);
    strcat(fullurl,"://");
    strcat(fullurl,host);
    strcat(fullurl,":");
    strcat(fullurl,port);
    strcat(fullurl,"/onvif/device_service");

    C_INFO("Manually adding URL : '%s'",fullurl);

    OnvifDevice * onvif_dev = OnvifDevice__new((char *)fullurl);
    if(!OnvifDevice__is_valid(onvif_dev)){
        C_ERROR("Invalid URL provided\n");
        g_object_unref(onvif_dev);
        goto exit;
    }
    OnvifDevice__set_credentials(onvif_dev,OnvifMgrAddDialog__get_user(dialog),OnvifMgrAddDialog__get_pass(dialog));
   
    /* Authentication check */
    GtkWidget * omgr_device;

    SoapFault fault = OnvifDevice__authenticate(onvif_dev);
    if(QueueEvent__is_cancelled(qevt)){
        g_object_unref(onvif_dev);
        goto exit;
    }

    OnvifApp * app = NULL;
    g_object_get (G_OBJECT (dialog), "userdata", &app, NULL);

    switch(fault){
        case SOAP_FAULT_NONE:
            //Extract scope
            omgr_device = OnvifMgrDeviceRow__new(app, onvif_dev, NULL, NULL, NULL);
            OnvifMgrDeviceRow__load_scopedata(ONVIFMGR_DEVICEROW(omgr_device));
            gdk_threads_add_idle(G_SOURCE_FUNC(idle_add_device),omgr_device);
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
            g_object_set (dialog, "error", "Failed to connect...", NULL);
            goto exit;
            break;
        case SOAP_FAULT_NOT_VALID:
            g_object_set (dialog, "error", "Not a valid ONVIF device...", NULL);
            goto exit;
            break;
        case SOAP_FAULT_UNAUTHORIZED:
            g_object_set (dialog, "error", "Unauthorized...", NULL);
            goto exit;
            break;
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            C_ERROR("An soap error was encountered");
            g_object_set (dialog, "error", "Unexected error occured...", NULL);
            goto exit;
            break;
    }

    safely_destroy_widget(GTK_WIDGET(dialog));
    return;
exit:
    gdk_threads_add_idle(G_SOURCE_FUNC(idle_hide_dialog_loading),dialog);
}


static gboolean OnvifApp__disocvery_found_server_cb (DiscoveryEvent * event) {
    C_TRACE("OnvifApp__found_server_cb");
    DiscoveredServer * server = event->server;
    OnvifApp * app = (OnvifApp *) event->data;
    if(!COwnableObject__has_owner(COWNABLE_OBJECT(app))){
        goto exit;
    }

    ProbMatch * m;
    int i;
  
    C_INFO("Found server - Match count : %i\n",server->matches->match_count);
    for (i = 0 ; i < server->matches->match_count ; ++i) {
        m = server->matches->matches[i];
        if(!OnvifApp__device_already_exist(app,m->addrs[0])){
            gchar * addr_dup = g_strdup(m->addrs[0]);
            OnvifDevice * onvif_dev = OnvifDevice__new(addr_dup);
            free(addr_dup);
            
            char * name = onvif_extract_scope("name",m);
            char * hardware = onvif_extract_scope("hardware",m);
            char * location = onvif_extract_scope("location",m);
            
            GtkWidget * omgr_device = OnvifMgrDeviceRow__new(app,onvif_dev,name,hardware,location);
            OnvifApp__add_device(app,ONVIFMGR_DEVICEROW(omgr_device));
            free(name);
            free(hardware);
            free(location);
        }
    }

exit:
    CObject__destroy((CObject*)event);
    return FALSE;
}

void OnvifApp__cred_dialog_login_cb(OnvifMgrAppDialog * app_dialog, OnvifMgrDeviceRow * device){
    C_INFO("OnvifAuthentication attempt...\n");
    OnvifMgrCredentialsDialog * cred_dialog = ONVIFMGR_CREDENTIALSDIALOG(app_dialog);
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (OnvifMgrDeviceRow__get_app(device));
    OnvifMgrAppDialog__show_loading(app_dialog,"ONVIF Authentication attempt...");
    OnvifDevice__set_credentials(OnvifMgrDeviceRow__get_device(device),OnvifMgrCredentialsDialog__get_username(cred_dialog),OnvifMgrCredentialsDialog__get_password(cred_dialog));
    g_object_ref(device);
    EventQueue__insert(priv->queue, device, _onvif_authentication_reload,app_dialog, _onvif_authentication_reload_cleanup);
}

void OnvifApp__cred_dialog_cancel_cb(OnvifMgrAppDialog * app_dialog, OnvifMgrDeviceRow * device){
    C_INFO("OnvifAuthentication cancelled...");
    g_object_unref(device);
}

void OnvifApp__player_retry_cb(GstRtspPlayer * player, GstRtspPlayerSession * session, void * user_data){
    C_TRACE("OnvifApp__player_retry_cb");
    OnvifApp * self = ONVIFMGR_APP(user_data);
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(GstRtspPlayerSession__get_user_data(session));
    //Check if the device is valid and selected
    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) && OnvifMgrDeviceRow__is_selected(device)){
        struct IdleRetryData * data = malloc(sizeof(struct IdleRetryData));
        data->player = player;
        data->session = session;
        data->device = device;
        EventQueue__insert_plain(priv->queue, device, _wait_before_retry_stream,data, NULL);
    }
}

void OnvifApp__player_error_cb(GstRtspPlayer * player, GstRtspPlayerSession * session, void * user_data){
    C_ERROR("Stream encountered an error");
    OnvifApp * self = (OnvifApp *) user_data;
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    //On shutdown, the player may dispatch this event after the window is destroyed
    if(GTK_IS_SPINNER(priv->player_loading_handle)){
        gtk_spinner_stop (GTK_SPINNER (priv->player_loading_handle));
    }
}

void OnvifApp__player_stopped_cb(GstRtspPlayer * player, void * user_data){
    C_INFO("Stream stopped");
    //TODO Show placeholder on canvas
}

void OnvifApp__player_started_cb(GstRtspPlayer * player, GstRtspPlayerSession * session, void * user_data){
    C_INFO("Stream playing");
    OnvifApp * self = (OnvifApp *) user_data;
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    if(GTK_IS_SPINNER(priv->player_loading_handle)){
        gtk_spinner_stop (GTK_SPINNER (priv->player_loading_handle));
    }
}

void OnvifApp__eq_dispatch_cb(EventQueue * queue, QueueEventType type, int running, int pending, int threadcount, QueueEvent * evt, OnvifApp * self){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);

    C_TRACE("EventQueue %s : %d/%d/%d",g_enum_to_nick(QUEUE_TYPE_EVENTTYPE,type),running,pending,threadcount);

    if(!GTK_IS_WIDGET(priv->task_label)){
        return;
    }
    
    char str[10];
    memset(&str,'\0',sizeof(str));
    sprintf(str, "[%d/%d]", running + pending,threadcount);

    gui_set_label_text(priv->task_label,str);
}

void OnvifApp__setting_view_mode_cb(AppSettingsStream * settings, GParamSpec* pspec, OnvifApp * app){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    int view_mode = AppSettingsStream__get_view_mode(settings);

    //TODO Support native mode
    if(!view_mode){
        GstRtspPlayer__set_view_mode(priv->player,GST_RTSP_PLAYER_VIEW_MODE_FIT_WINDOW);
    } else {
        GstRtspPlayer__set_view_mode(priv->player,GST_RTSP_PLAYER_VIEW_MODE_FILL_WINDOW);
    }
}

void OnvifApp__profile_selected_cb(OnvifMgrProfilesDialog * profile_dialog, OnvifMediaProfile * profile, OnvifMgrDeviceRow * device){
    OnvifMgrDeviceRow__set_profile(device,profile);
}

/* This function is called when the STOP button is clicked */
static void OnvifApp__quit_cb (GtkButton *button, OnvifApp *data) {
    onvif_app_shutdown(data);
}

/* This function is called when the main window is closed */
static void OnvifApp__window_delete_event_cb (GtkWidget *widget, GdkEvent *event, OnvifApp *data) {
    onvif_app_shutdown(data);
}

void OnvifApp__btn_scan_cb (GtkWidget *widget, OnvifApp * app) {
    C_INFO("Starting ONVIF Devices Network Discovery...");
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);

    gtk_widget_set_sensitive(widget,FALSE);

    //Only need to cancel scope if list is cleared.
    //TODO Cancel scope when removing individual devices
    // //Cancell all pending events associated to devices removed.
    // GList * childs = gtk_container_get_children(GTK_CONTAINER(priv->listbox));
    // GList * tmp = childs;
    // int scopes_count = 0;
    // void ** scopes = NULL;
    // OnvifMgrDeviceRow * device;
    // GLIST_FOREACH(device, childs) {
    //     scopes_count++;
    //     if(!scopes){
    //         scopes = malloc(sizeof(void*)*scopes_count);
    //     } else {
    //         scopes = realloc(scopes,sizeof(void*)*scopes_count);
    //     }
    //     scopes[scopes_count-1] = device;
    //     childs = childs->next;
    // }
    // g_list_free(tmp);

    // //Submit cancellation request
    // EventQueue__cancel_scopes(priv->queue,scopes, scopes_count);
    // free(scopes);

    //Clearing the list
    // gtk_container_foreach (GTK_CONTAINER (priv->listbox), (GtkCallback)gui_container_remove, priv->listbox);

    //Multiple dispatch in case of packet dropped
    EventQueue__insert(priv->queue, app, _start_onvif_discovery,app, NULL);
}

static void OnvifApp__profile_picker_cb (OnvifMgrDeviceRow *device){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (OnvifMgrDeviceRow__get_app(device));

    OnvifMgrProfilesDialog * profile_dialog = OnvifMgrProfilesDialog__new(priv->queue,device);
    g_signal_connect (G_OBJECT (profile_dialog), "profile-selected", G_CALLBACK (OnvifApp__profile_selected_cb), device);
    gtk_overlay_add_overlay(priv->overlay,GTK_WIDGET(profile_dialog));
    gtk_widget_show_all(GTK_WIDGET(priv->overlay));
}

static void OnvifApp__profile_changed_cb (OnvifMgrDeviceRow *device){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (OnvifMgrDeviceRow__get_app(device));
    EventQueue__insert(priv->queue, device, _profile_callback,device, NULL);
}
void OnvifApp__add_device_cb(OnvifMgrAppDialog * app_dialog, OnvifApp * app){
    const char * host = OnvifMgrAddDialog__get_host(ONVIFMGR_ADDDIALOG(app_dialog));
    if(!host || strlen(host) < 2){
        C_WARN("Host field invalid. (too short)");
        return;
    }

    OnvifMgrAppDialog__show_loading(app_dialog, "Testing ONVIF device configuration...");
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    EventQueue__insert(priv->queue, app, _onvif_device_add,app_dialog, NULL);
}

void OnvifApp__add_btn_cb (GtkWidget *widget, OnvifApp * app) {
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    OnvifMgrAddDialog * dialog = OnvifMgrAddDialog__new();
    g_object_set (dialog, "userdata", app, NULL);
    g_signal_connect (G_OBJECT (dialog), "submit", G_CALLBACK (OnvifApp__add_device_cb), app);
    gtk_overlay_add_overlay(priv->overlay,GTK_WIDGET(dialog));
    gtk_widget_show_all(GTK_WIDGET(priv->overlay));
}

void OnvifApp__row_selected_cb (GtkWidget *widget,   GtkListBoxRow* row, OnvifApp* app){
    OnvifApp__select_device(app,row);
}

static gboolean OnvifApp__discovery_finished_cb (OnvifApp * self) {
    C_TRACE("OnvifApp__discovery_finished_cb");
    if(!COwnableObject__has_owner(COWNABLE_OBJECT(self))){
        goto exit;
    }

    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);

    if(GTK_IS_WIDGET(priv->btn_scan)){
        gtk_widget_set_sensitive(priv->btn_scan,TRUE);
    }

exit:
    g_object_unref(self);
    return FALSE;
}


static SoapFault OnvifApp__reload_device(QueueEvent * qevt, OnvifMgrDeviceRow * device){
    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);

    /* Authentication check */
    SoapFault fault = OnvifDevice__authenticate(odev);
    
    //Check if device is valid and authorized (User performed scan before auth finished)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifDevice__is_authenticated(odev) || QueueEvent__is_cancelled(qevt)) {
        return fault;
    }

    //Replace locked image with spinner
    GtkWidget * image = gtk_spinner_new ();
    OnvifMgrDeviceRow__set_thumbnail(device,image);

    OnvifApp * app = OnvifMgrDeviceRow__get_app(device);
    OnvifApp__display_device(app, device);

    if(OnvifDevice__is_authenticated(odev) && OnvifMgrDeviceRow__is_selected(device)){
        //Authenticated device counts a changed
        gui_signal_emit(app, signals[DEVICE_CHANGED], device);

        OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
        safely_start_spinner(priv->player_loading_handle);
        _play_onvif_stream(qevt, device);
    }

    return fault;
}

gboolean OnvifApp__set_device(OnvifApp * app, GtkListBoxRow * row){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    if(!ONVIFMGR_IS_DEVICEROW(row)) {
        C_DEBUG("OnvifApp__set_device - NULL");
        if(ONVIFMGR_IS_DEVICEROW(priv->device)) g_object_unref(priv->device);
        priv->device = NULL;
        return FALSE; 
    }

    OnvifMgrDeviceRow * old = priv->device;

    C_DEBUG("OnvifApp__set_device - valid device");
    priv->device = ONVIFMGR_DEVICEROW(row);

    //If changing device, trade reference
    if(ONVIFMGR_DEVICEROW(old) != priv->device) {
        if(G_IS_OBJECT(old))
            g_object_unref(old);
        g_object_ref(priv->device);
    }
    return TRUE;
}

static void OnvifApp__select_device(OnvifApp * app,  GtkListBoxRow * row){
    C_DEBUG("OnvifApp__select_device");
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);

    //Stop previous stream
    EventQueue__insert(priv->queue,app, _stop_onvif_stream,app, NULL);

    if(!OnvifApp__set_device(app,row)){
        //In case the previous stream was in a retry cycle, force hide loading
        gtk_spinner_stop (GTK_SPINNER (priv->player_loading_handle));
    } else if(OnvifMgrDeviceRow__is_initialized(priv->device)){
        OnvifDevice * odev = OnvifMgrDeviceRow__get_device(priv->device);
        //Prompt for authentication
        if(!OnvifDevice__is_authenticated(odev)){
            //In case the previous stream was in a retry cycle, force hide loading
            gtk_spinner_stop (GTK_SPINNER (priv->player_loading_handle));
            //Re-dispatched to allow proper focus handling
            g_object_ref(priv->device);
            gdk_threads_add_idle(G_SOURCE_FUNC(idle_show_credentialsdialog),priv->device);
            goto exit;
        }

        gtk_spinner_start (GTK_SPINNER (priv->player_loading_handle));
        EventQueue__insert(priv->queue, priv->device, _play_onvif_stream,priv->device, NULL);
    }

exit:
    g_signal_emit (app, signals[DEVICE_CHANGED], 0, ONVIFMGR_DEVICEROW(row) /* details */);
}

static void OnvifApp__display_device(OnvifApp * self, OnvifMgrDeviceRow * device){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    EventQueue__insert(priv->queue, device, _display_onvif_device,device, NULL);
}

static void OnvifApp__add_device(OnvifApp * app, OnvifMgrDeviceRow * omgr_device){
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    g_signal_connect (G_OBJECT (omgr_device), "profile-clicked", G_CALLBACK (OnvifApp__profile_picker_cb), NULL);

    gtk_list_box_insert (GTK_LIST_BOX (priv->listbox), GTK_WIDGET(omgr_device), -1);
    gtk_widget_show_all (GTK_WIDGET(omgr_device));

    OnvifApp__display_device(app,omgr_device);
}

static int OnvifApp__device_already_exist(OnvifApp * app, char * xaddr){
    // if(strcmp(xaddr,"device_service URL") == 0 ||
    //     strcmp(xaddr,"device_service URL2") == 0 ){
    //     return 1;
    // }
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    int ret = 0;
    GList * childs = gtk_container_get_children(GTK_CONTAINER(priv->listbox));
    GList * tmp = childs;
    OnvifMgrDeviceRow * device;
    GLIST_FOREACH(device, childs) {
        OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);
        OnvifDeviceService * devserv = OnvifDevice__get_device_service(odev);
        char * endpoint = OnvifBaseService__get_endpoint(ONVIF_BASE_SERVICE(devserv));
        if(!strcmp(xaddr,endpoint)){
            C_DEBUG("Record already part of list [%s]\n",endpoint);
            ret = 1;
        }
        free(endpoint);
    }
    g_list_free(tmp);
    return ret;
}

GtkWidget * OnvifApp__create_browser_ui(OnvifApp * app){
    GtkWidget * grid;
    GtkWidget *widget;
    GtkWidget *label;

    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);
    grid = gtk_grid_new ();
    GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0); //Horizontal container for Scan and Add buttons
    gtk_grid_attach (GTK_GRID (grid), hbox, 0, 0, 1, 1);

    char * btn_css = "* { padding: 0px; font-size: 1.5em; }";

    GError *error = NULL;
    GtkWidget * image = GtkStyledImage__new((unsigned char *)_binary_tower_png_start, _binary_tower_png_end - _binary_tower_png_start, 25, 25, error);

    priv->btn_scan = gtk_button_new ();
    // label = gtk_label_new("");
    // gtk_label_set_markup (GTK_LABEL (label), "<span><b><b><b>Scan</b></b></b>...</span>");
    // gtk_label_set_markup (GTK_LABEL (label), "&#x1F50D;"); //Magnifying glass (color)
    // gtk_label_set_markup (GTK_LABEL (label), "&#x1f4e1;"); //Antenna arrow (color)
    // gtk_label_set_markup (GTK_LABEL (label), "&#x2609;"); //Dotted circle (called sun)
    // gtk_label_set_markup (GTK_LABEL (label), "&#x27F2;"); //clockwise Circle arrow (Safe icon theme expectation)
    // gtk_label_set_markup (GTK_LABEL (label), "&#x1f78b;"); //Target (Not center aligned)
    // gtk_container_add (GTK_CONTAINER (app->btn_scan), label);
    if(image){
        gtk_button_set_image(GTK_BUTTON(priv->btn_scan), image);
    } else {
        if(error && error->message){
            C_ERROR("Error creating tower icon : %s",error->message);
        } else {
            C_ERROR("Error creating tower icon : [null]");
        }
        label = gtk_label_new("");
        gtk_label_set_markup (GTK_LABEL (label), "&#x27F2;"); //clockwise Circle arrow (Safe icon theme expectation)
        gtk_container_add (GTK_CONTAINER (priv->btn_scan), label);
    }

    gtk_box_pack_start (GTK_BOX(hbox),priv->btn_scan,TRUE,TRUE,0);
    g_signal_connect (priv->btn_scan, "clicked", G_CALLBACK (OnvifApp__btn_scan_cb), app);
    gui_widget_set_css(priv->btn_scan, btn_css);


    GtkWidget * add_btn = gtk_button_new ();
    gui_widget_make_square(add_btn);
    label = gtk_label_new("");
    // gtk_label_set_markup (GTK_LABEL (label), "&#x2795;"); //Heavy
    gtk_label_set_markup (GTK_LABEL (label), "&#xFF0B;"); //Full width
    gtk_container_add (GTK_CONTAINER (add_btn), label);
    gui_widget_set_css(add_btn, btn_css);

    gtk_box_pack_start (GTK_BOX(hbox),add_btn,FALSE,FALSE,0);

    g_signal_connect (add_btn, "clicked", G_CALLBACK (OnvifApp__add_btn_cb), app);

    /* --- Create a list item from the data element --- */
    widget = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (widget, TRUE);
    priv->listbox = gtk_list_box_new ();
    //TODO Calculate preferred width to display IP without scrollbar
    gtk_widget_set_size_request(widget,200,-1);
    gtk_widget_set_vexpand (priv->listbox, TRUE);
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (priv->listbox), GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(widget),priv->listbox);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);
    g_signal_connect (priv->listbox, "row-selected", G_CALLBACK (OnvifApp__row_selected_cb), app);
    gui_widget_set_css(widget, "* { padding-bottom: 3px; padding-top: 3px; padding-right: 3px; }"); 

    widget = gtk_button_new ();
    label = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (label), "<span size=\"medium\">Quit</span>");
    gtk_container_add (GTK_CONTAINER (widget), label);
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_widget_set_hexpand (widget, FALSE);
    g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK (OnvifApp__quit_cb), app);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 3, 1, 1);
    gui_widget_set_css(widget, btn_css);

    return grid;
}

void OnvifApp__create_ui (OnvifApp * app) {
    GtkWidget *main_window;  /* The uppermost window, containing all other windows */
    GtkWidget *widget;
    GtkWidget *label;
    GtkWidget * hbox;
    GtkWidget *main_notebook;

    OnvifAppPrivate *priv = OnvifApp__get_instance_private (app);

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (OnvifApp__window_delete_event_cb), app);

    gtk_window_set_title (GTK_WINDOW (main_window), "Onvif Device Manager");

    priv->overlay = GTK_OVERLAY(gtk_overlay_new());
    gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET(priv->overlay));

    //Division between the player and camera browser
    GtkWidget *hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_container_set_border_width (GTK_CONTAINER (hpaned), 5);
    gtk_widget_set_vexpand (hpaned, TRUE);
    gtk_widget_set_hexpand (hpaned, TRUE);
    gui_widget_set_css(hpaned, "paned separator{min-width: 10px;}");
    //Add camera browser
    gtk_paned_pack1 (GTK_PANED (hpaned), OnvifApp__create_browser_ui(app), FALSE, FALSE);

    /* Create a new notebook, place the position of the tabs */
    main_notebook = gtk_notebook_new ();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(main_notebook),TRUE);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (main_notebook), GTK_POS_TOP);
    gtk_widget_set_vexpand (main_notebook, TRUE);
    gtk_widget_set_hexpand (main_notebook, TRUE);
    gtk_paned_pack2 (GTK_PANED (hpaned), main_notebook, TRUE, FALSE);
    gtk_overlay_add_overlay(priv->overlay,hpaned);
    // gtk_grid_attach (GTK_GRID (grid), hpaned, 0, 0, 1, 4);
    gtk_paned_set_wide_handle(GTK_PANED(hpaned),TRUE);
    char * TITLE_STR = "NVT";
    label = gtk_label_new (TITLE_STR);

    //Hidden spinner used to display stream start loading
    priv->player_loading_handle = gtk_spinner_new ();

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),priv->player_loading_handle,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = OnvifNVT__create_ui(priv->player);
    gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), widget, hbox);

    label = gtk_label_new ("Details");
    //Hidden spinner used to display stream start loading
    widget = gtk_spinner_new ();
    OnvifDetails__set_details_loading_handle(priv->details,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = OnvifDetails__get_widget(priv->details);

    gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), widget, hbox);


    label = gtk_label_new ("Settings");
    //Hidden spinner used to display stream start loading
    widget = gtk_spinner_new ();
    AppSettings__set_details_loading_handle(priv->settings,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = AppSettings__get_widget(priv->settings);

    gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), widget, hbox);



    label = gtk_label_new ("Task Manager");
    //Hidden spinner used to display stream start loading
    // widget = gtk_spinner_new ();
    // TaskMgr__set_loading_handle(app->taskmgr,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);

    priv->task_label = gtk_label_new ("[0/0]");
    gtk_box_pack_start(GTK_BOX(hbox),priv->task_label,FALSE,FALSE,0);
    // gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), GTK_WIDGET(OnvifTaskManager__new(priv->queue)), hbox);


    GdkMonitor* monitor = gdk_display_get_primary_monitor(gdk_display_get_default());
    if(monitor){
        GdkRectangle workarea = {0};
        gdk_monitor_get_workarea(monitor,&workarea);
        
        if(workarea.width > 775){//If resolution allows it, big enough to show no scrollbars
            gtk_window_set_default_size(GTK_WINDOW(main_window),775,480);
        } else {//Resolution is so low that we launch fullscreen
            gtk_window_set_default_size(GTK_WINDOW(main_window),workarea.width,workarea.height);
        }
    } else {
        gtk_window_set_default_size(GTK_WINDOW(main_window),540,380);
    }

    priv->window = main_window;
    gtk_widget_show_all (main_window);
}

void OnvifApp__dispose(GObject * obj){
    OnvifApp * self = ONVIFMGR_APP(obj);
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);

    //Destroying the queue will hang until all threads are stopped
    if(priv->queue){
        g_object_unref(priv->queue);
        priv->queue = NULL;
    }

    if(priv->details){
        OnvifDetails__destroy(priv->details);
        priv->details = NULL;
    }

    if(priv->settings){
        AppSettings__destroy(priv->settings);
        priv->settings = NULL;
    }

    //Destroying the player will cause it to hang until its state changed to NULL
    //Destroying the player after the queue because the queue could dispatch retry call
    if(priv->player){
        g_object_unref(priv->player);
        priv->player = NULL;
    }
    
    //Using idle destruction to allow pending idles to execute
    if(priv->window){
        safely_destroy_widget(priv->window);
        priv->window = NULL;
    }
}

static void
OnvifApp__class_init (OnvifAppClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = OnvifApp__dispose;

    // GType type = ONVIFMGR_TYPE_DEVICEROW;
    GType params[1];
    params[0] = ONVIFMGR_TYPE_DEVICEROW | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[DEVICE_CHANGED] =
        g_signal_newv ("device-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);
}

static gboolean
OnvifApp_has_owner (COwnableObject *self)
{
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (ONVIFMGR_APP(self));
    return priv->owned;
}

static void
OnvifApp_disown (COwnableObject *self)
{
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (ONVIFMGR_APP(self));
    priv->owned = FALSE;
    g_object_unref(self);
}

static void
OnvifApp__ownable_interface_init (COwnableObjectInterface *iface)
{
    iface->has_owner = OnvifApp_has_owner;
    iface->disown = OnvifApp_disown;
}

static void
OnvifApp__cancelled_store (OnvifMgrEncryptedStore * store, OnvifApp *self){
    //TODO Support for no credentials storage file
    C_WARN("Credentials Store dialog cancelled.");
    onvif_app_shutdown(self);
}

static void
OnvifApp__new_object_store(OnvifMgrEncryptedStore * store, OnvifMgrSerializable * serializable, OnvifApp * app){
    if(ONVIFMGR_IS_DEVICEROW(serializable)){
        OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(serializable);
        ONVIFMGR_DEVICEROW_DEBUG("[%s] Found device from stoage.", device);
        g_object_set(device, "app", app, NULL);
        OnvifApp__add_device(app,ONVIFMGR_DEVICEROW(device));
    } else {
        C_ERROR("Unknown object type found in store file.");
    }
}

static void
OnvifApp__init (OnvifApp *self)
{
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);

    priv->device = NULL;
    priv->owned = 1;
    priv->task_label = NULL;
    priv->queue = EventQueue__new();
    g_signal_connect (G_OBJECT(priv->queue), "pool-changed", G_CALLBACK (OnvifApp__eq_dispatch_cb), self);

    //TODO register listener
    priv->details = OnvifDetails__create(self);
    priv->settings = AppSettings__create(self);

    g_signal_connect (priv->settings->stream, "notify::view-mode", G_CALLBACK (OnvifApp__setting_view_mode_cb), self);

    priv->player = GstRtspPlayer__new();
    OnvifApp__setting_view_mode_cb(priv->settings->stream, NULL, self);

    g_signal_connect (G_OBJECT(priv->player), "retry", G_CALLBACK (OnvifApp__player_retry_cb), self);
    g_signal_connect (G_OBJECT(priv->player), "error", G_CALLBACK (OnvifApp__player_error_cb), self);
    g_signal_connect (G_OBJECT(priv->player), "stopped", G_CALLBACK (OnvifApp__player_stopped_cb), self);
    g_signal_connect (G_OBJECT(priv->player), "started", G_CALLBACK (OnvifApp__player_started_cb), self);
    
    OnvifApp__create_ui (self);
    priv->store = OnvifMgrEncryptedStore__new(priv->queue,priv->overlay);
    g_signal_connect (G_OBJECT (priv->store), "cancel", G_CALLBACK (OnvifApp__cancelled_store), self);
    g_signal_connect (G_OBJECT (priv->store), "new-object", G_CALLBACK (OnvifApp__new_object_store), self);
    OnvifMgrEncryptedStore__capture_passphrase(priv->store);

    //Defaults 8 paralell event threads.
    //TODO support configuration to modify this
    C_TRAIL("Starting EventQueue thread pool...");
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);
    EventQueue__start(priv->queue);

}


OnvifApp * OnvifApp__new(void){
    return g_object_new (ONVIFMGR_TYPE_APP, NULL);
}

void OnvifApp__destroy(OnvifApp* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_APP (self));
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    
    GList * childs = gtk_container_get_children(GTK_CONTAINER(priv->listbox));
    if(childs){
        int data_saved = OnvifMgrEncryptedStore__save(priv->store,childs);
        if(data_saved <= 0){
            C_ERROR("Failed to save camera details.");
        }
        g_list_free(childs);
    }


    g_object_ref(self);
    COwnableObject__disown(COWNABLE_OBJECT(self)); //Changing owned state, flagging pending events to be ignored
    g_object_unref(self);
}

void OnvifApp__show_msg_dialog(OnvifApp * self, OnvifMgrMsgDialog * msg_dialog){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_APP (self));

    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    gtk_overlay_add_overlay(priv->overlay,GTK_WIDGET(msg_dialog));
    gtk_widget_show_all(GTK_WIDGET(priv->overlay));
}

EventQueue * OnvifApp__get_EventQueue(OnvifApp* self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_APP (self), NULL);
    OnvifAppPrivate *priv = OnvifApp__get_instance_private (self);
    return priv->queue;
}