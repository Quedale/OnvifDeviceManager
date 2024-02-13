#include "onvif_app.h"
#include "dialog/credentials_input.h"
#include "dialog/add_device.h"
#include "onvif_details.h"
#include "onvif_nvt.h"
#include "gui_utils.h"
#include "settings/app_settings.h"
#include "../queue/event_queue.h"
#include "discoverer.h"
#include "task_manager.h"
#include "clogger.h"
#include "../animations/gtk/gtk_dotted_slider_widget.h"
#include "omgr_device_row.h"

typedef struct _OnvifApp {
    OnvifMgrDeviceRow * device;

    GtkWidget *btn_scan;
    GtkWidget *window;
    GtkWidget *listbox;
    GtkWidget *main_notebook;
    GtkWidget *player_loading_handle;
    GtkWidget *task_label;

    ProfilesDialog * profiles_dialog;
    CredentialsDialog * cred_dialog;
    AddDeviceDialog * add_dialog;
    MsgDialog * msg_dialog;

    int current_page;

    OnvifDetails * details;
    AppSettings * settings;
    TaskMgr * taskmgr;

    EventQueue * queue;
    RtspPlayer * player;
} OnvifApp;

void stopped_onvif_stream(RtspPlayer * player, void * user_data);
void _profile_callback (void * user_data);
void add_device(OnvifMgrDeviceRow * device);
gboolean OnvifApp__set_device(OnvifApp * app, GtkListBoxRow * row);

/*
 *
 *  Gtk GUI Signal Callbacks
 *
 */

/* This function is called when the STOP button is clicked */
static void quit_cb (GtkButton *button, OnvifApp *data) {
    onvif_app_shutdown(data);

}

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, OnvifApp *data) {
    onvif_app_shutdown(data);
}

static int device_already_found(OnvifApp * app, char * xaddr){
    int ret = 0;
    GList * childs = gtk_container_get_children(GTK_CONTAINER(app->listbox));
    OnvifMgrDeviceRow * device;
    GLIST_FOREACH(device, childs) {
        OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);
        OnvifDeviceService * devserv = OnvifDevice__get_device_service(odev);
        char * endpoint = OnvifDeviceService__get_endpoint(devserv);
        if(!strcmp(xaddr,endpoint)){
            C_DEBUG("Record already part of list [%s]\n",endpoint);
            ret = 1;
        }
        free(endpoint);
        childs = childs->next;
    }
    g_list_free(childs);
    return ret;
}

static gboolean * found_server (void * e) {
    C_TRACE("found_server");
    DiscoveryEvent * event = (DiscoveryEvent *) e;

    DiscoveredServer * server = event->server;
    OnvifApp * app = (OnvifApp *) event->data;

    ProbMatch * m;
    int i;
  
    C_INFO("Found server - Match count : %i\n",server->matches->match_count);
    for (i = 0 ; i < server->matches->match_count ; ++i) {
        m = server->matches->matches[i];
        if(!device_already_found(app,m->addrs[0])){
            gchar * addr_dup = g_strdup(m->addrs[0]);
            OnvifDevice * onvif_dev = OnvifDevice__create(addr_dup);
            free(addr_dup);
            
            char * name = onvif_extract_scope("name",m);
            char * hardware = onvif_extract_scope("hardware",m);
            char * location = onvif_extract_scope("location",m);
            
            GtkWidget * omgr_device = OnvifMgrDeviceRow__new(app,onvif_dev,name,hardware,location);
            add_device(ONVIFMGR_DEVICEROW(omgr_device));
            free(name);
            free(hardware);
            free(location);
        }
    }

    CObject__destroy((CObject*)event);
    return FALSE;
}

void _found_server (DiscoveryEvent * event) {
    C_TRACE("_found_server");
    gdk_threads_add_idle(G_SOURCE_FUNC(found_server),event);
}

gboolean * finished_discovery (void * e) {
    C_TRACE("finished_discovery");
    OnvifApp * app = (OnvifApp * ) e;

    if(GTK_IS_WIDGET(app->btn_scan)){
        gtk_widget_set_sensitive(app->btn_scan,TRUE);
    }

    return FALSE;
}

void _start_onvif_discovery(void * user_data){
    C_TRACE("_start_onvif_discovery");
    OnvifApp * app = (OnvifApp *) user_data;
    //Start UDP Scan
    struct UdpDiscoverer discoverer = UdpDiscoverer__create(_found_server);
    UdpDiscoverer__start(&discoverer, app, AppSettingsDiscovery__get_repeat(app->settings->discovery), AppSettingsDiscovery__get_timeout(app->settings->discovery));
    
    gdk_threads_add_idle(G_SOURCE_FUNC(finished_discovery),app);
}

void onvif_scan (GtkWidget *widget, OnvifApp * app) {
    C_INFO("Starting ONVIF Devices Network Discovery...");
    gtk_widget_set_sensitive(widget,FALSE);
    //Between retries, player may not send stopped event since the state didn't stop
    //Forcing hiding the loading indicator
    stopped_onvif_stream(app->player,app);

    //Clearing the list
    gtk_container_foreach (GTK_CONTAINER (app->listbox), (GtkCallback)gui_container_remove, app->listbox);
    // EventQueue__clear(app->queue); //Unable to safely clear because pointers are released inside pending events

    //Multiple dispatch in case of packet dropped
    EventQueue__insert(app->queue,_start_onvif_discovery,app);
}

void error_onvif_stream(RtspPlayer * player, void * user_data){
    C_INFO("Stream encountered an error\n");
    OnvifApp * app = (OnvifApp *) user_data;
    //On shutdown, the player may dispatch this event after the window is destroyed
    if(GTK_IS_SPINNER(app->player_loading_handle)){
        gtk_spinner_stop (GTK_SPINNER (app->player_loading_handle));
    }
}

void stopped_onvif_stream(RtspPlayer * player, void * user_data){
    C_INFO("Stream stopped\n");
    //TODO Show placeholder on canvas
}

void started_onvif_stream(RtspPlayer * player, void * user_data){
    C_INFO("Stream playing\n");
    OnvifApp * app = (OnvifApp *) user_data;
    if(GTK_IS_SPINNER(app->player_loading_handle)){
        gtk_spinner_stop (GTK_SPINNER (app->player_loading_handle));
    }
}

void _retry_onvif_stream(void * user_data){
    C_TRACE("_retry_onvif_stream");
    OnvifApp * app = (OnvifApp *) user_data;
    sleep(2);
    RtspPlayer__retry(app->player);
}

void retry_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    EventQueue__insert(app->queue,_retry_onvif_stream,app);
}

void _stop_onvif_stream(void * user_data){
    C_TRACE("_stop_onvif_stream");
    OnvifApp * app = (OnvifApp *) user_data;
    RtspPlayer__stop(app->player);
}

static void device_profile_changed (OnvifMgrDeviceRow *device){
    g_object_ref(device);
    EventQueue__insert(OnvifMgrDeviceRow__get_app(device)->queue,_profile_callback,device);
}

void _display_onvif_device(void * user_data){
    C_TRACE("_display_onvif_device");
    OnvifMgrDeviceRow * omgr_device = (OnvifMgrDeviceRow *) user_data;
    COwnableObject__has_owner(COWNABLE_OBJECT(omgr_device));
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(omgr_device)){
        C_TRAIL("_display_onvif_device - invalid device.");
        goto exit;
    }
    
    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(omgr_device);

    /* Authentication check */
    OnvifDevice__authenticate(odev);
        

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(omgr_device)){
        C_TRAIL("_display_onvif_device - invalid device.");
        goto exit;
    } else if (OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NOT_AUTHORIZED){
        goto updatethumb;
    }

    /* Display Profile dropdown */
    if(!OnvifMgrDeviceRow__get_profile(omgr_device)){
        OnvifProfiles * profiles = OnvifMediaService__get_profiles(OnvifDevice__get_media_service(odev));
        OnvifMgrDeviceRow__set_profile(omgr_device,OnvifProfiles__get_profile(profiles,0));
        OnvifProfiles__destroy(profiles);
        //We don't care for the initial profile event since the default index is 0.
        //Connecting to signal only after setting the default profile
        g_signal_connect (G_OBJECT (omgr_device), "profile-changed", G_CALLBACK (device_profile_changed), NULL);

    }

    /* Display row thumbnail. Default to profile index 0 */
updatethumb:
    OnvifMgrDeviceRow__load_thumbnail(omgr_device);
exit:
    g_object_unref(omgr_device);
}

void onvif_display_device_row(OnvifApp * self, OnvifMgrDeviceRow * device){
    g_object_ref(device);
    EventQueue__insert(self->queue,_display_onvif_device,device);
}

void _play_onvif_stream(void * user_data){
    ONVIFMGR_DEVICEROW_TRACE("_play_onvif_stream %s",user_data);
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);

    //Check if device is still valid. (User performed scan before thread started)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device)){
        C_TRAIL("_play_onvif_stream - invalid device.");
        goto exit;
    }

    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);
    OnvifApp * app = OnvifMgrDeviceRow__get_app(device);

    /* Authentication check */
    OnvifDevice__authenticate(odev);

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)) {
        C_TRAIL("_play_onvif_stream - invalid device.");
        goto exit;
    }
    if(OnvifDevice__get_last_error(odev) != ONVIF_ERROR_NONE && OnvifMgrDeviceRow__is_selected(device)){
        stopped_onvif_stream(app->player,app);
        goto exit;
    }

    /* Set the URI to play */
    char * uri = OnvifMediaService__getStreamUri(OnvifDevice__get_media_service(odev),OnvifProfile__get_index(OnvifMgrDeviceRow__get_profile(device)));
    
    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) && OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NONE){
        RtspPlayer__set_playback_url(app->player,uri);
        char * port = OnvifDevice__get_port(OnvifMgrDeviceRow__get_device(device));
        RtspPlayer__set_port_fallback(app->player,port);
        free(port);
        OnvifCredentials * ocreds = OnvifDevice__get_credentials(odev);
        RtspPlayer__set_credentials(app->player, OnvifCredentials__get_username(ocreds), OnvifCredentials__get_password(ocreds));

        RtspPlayer__play(app->player);
    } else if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)) {
        C_TRAIL("_play_onvif_stream - invalid device.");
    }
    free(uri);

exit:
    C_TRACE("_play_onvif_stream - done\n");
    g_object_unref(device);
}

//On the odd chance the by the time update_pages is invoked, the selection changed
//We use device as input parameters carried through the idle dispatch. Not from the app pointer
void update_pages(OnvifApp * self, OnvifMgrDeviceRow * device){
    //#0 NVT page already loaded
    //#2 Settings page already loaded
    //Details page is dynamically loaded
    if(self->current_page == 1){
        OnvifDetails__clear_details(self->details);
        OnvifDetails__update_details(self->details,device);
    }
}

gboolean * idle_update_pages(void * user_data){
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);
    OnvifApp * app = OnvifMgrDeviceRow__get_app(device);
    update_pages(app,device);
    g_object_unref(device);
    return FALSE;
}

int onvif_reload_device(OnvifMgrDeviceRow * device){
    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);

    /* Authentication check */
    OnvifDevice__authenticate(odev);
    
    //Check if device is valid and authorized (User performed scan before auth finished)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)) {
        C_TRAIL("onvif_reload_device - invalid device.");
        return 0;
    }

    if(OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NOT_AUTHORIZED){
        return 0;
    }

    //Replace locked image with spinner
    GtkWidget * image = gtk_spinner_new ();
    OnvifMgrDeviceRow__set_thumbnail(device,image);

    OnvifApp * app = OnvifMgrDeviceRow__get_app(device);
    onvif_display_device_row(app, device);

    g_object_ref(device);
    gdk_threads_add_idle(G_SOURCE_FUNC(idle_update_pages),device);
    if(OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NONE && OnvifMgrDeviceRow__is_selected(device)){
        gtk_spinner_start (GTK_SPINNER (app->player_loading_handle));
        g_object_ref(device);
        EventQueue__insert(app->queue,_play_onvif_stream,device);
    }

    return 1;
}

void _onvif_authentication_reload(void * user_data){
    AppDialogEvent * event = (AppDialogEvent *) user_data;
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(event->user_data);

    C_DEBUG("_onvif_authentication_reload\n");
    //Check device is still valid before adding ref (User performed scan before thread started)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)){
        C_TRAIL("_onvif_authentication_reload - invalid device\n");
        free(event);
        return;
    }

    if(onvif_reload_device(device)){
        AppDialog__hide((AppDialog*)OnvifMgrDeviceRow__get_app(device)->cred_dialog);
        g_object_unref(device);
    }
}

void dialog_login_cb(AppDialogEvent * event){
    C_INFO("OnvifAuthentication attempt...\n");
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(event->user_data);
    OnvifDevice__set_credentials(OnvifMgrDeviceRow__get_device(device),CredentialsDialog__get_username((CredentialsDialog*)event->dialog),CredentialsDialog__get_password((CredentialsDialog*)event->dialog));
    EventQueue__insert(OnvifMgrDeviceRow__get_app(device)->queue,_onvif_authentication_reload,AppDialogEvent_copy(event));
}

void dialog_cancel_cb(AppDialogEvent * event){
    C_INFO("OnvifAuthentication cancelled...\n");
    g_object_unref(event->user_data);
}

gboolean * idle_show_credentialsdialog (void * user_data){
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);
    AppDialog__show((AppDialog *) OnvifMgrDeviceRow__get_app(device)->cred_dialog,dialog_login_cb, dialog_cancel_cb,device);
    return FALSE;
}

void OnvifApp__select_device(OnvifApp * app,  GtkListBoxRow * row){
    C_DEBUG("OnvifApp__select_device");

    //Stop previous stream
    EventQueue__insert(app->queue,_stop_onvif_stream,app);

    //Clear details
    OnvifDetails__clear_details(app->details);

    if(!OnvifApp__set_device(app,row)){
        goto exit;
    }

    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(app->device);
    //Prompt for authentication
    if(OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NOT_AUTHORIZED){
        //Re-dispatched to allow proper focus handling
        g_object_ref(app->device);
        gdk_threads_add_idle(G_SOURCE_FUNC(idle_show_credentialsdialog),app->device);
        return;
    }

    gtk_spinner_start (GTK_SPINNER (app->player_loading_handle));
    g_object_ref(app->device);
    EventQueue__insert(app->queue,_play_onvif_stream,app->device);

exit:
    update_pages(app, app->device);
}

void row_selected_cb (GtkWidget *widget,   GtkListBoxRow* row, OnvifApp* app){
    OnvifApp__select_device(app,row);
}

static void switch_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifApp * app) {
    C_DEBUG("switch_page");
    app->current_page = page_num;
    update_pages(app, app->device);
}

gboolean idle_add_device(void * user_data){
    add_device(ONVIFMGR_DEVICEROW(user_data));
    return FALSE;
}

gboolean * idle_hide_dialog (void * user_data){
    AppDialog * dialog = (AppDialog *) user_data;
    AppDialog__hide(dialog);
    return FALSE;
}


void _onvif_device_add(void * user_data){
    MsgDialog * msgdialog;
    AppDialogEvent * event = (AppDialogEvent *) user_data;
    AddDeviceDialog * dialog = (AddDeviceDialog *) event->dialog;
    const char * host = AddDeviceDialog__get_host((AddDeviceDialog *)event->dialog);
    const char * port = AddDeviceDialog__get_port((AddDeviceDialog *)event->dialog);
    const char * protocol = AddDeviceDialog__get_protocol((AddDeviceDialog *)event->dialog);

    char fullurl[strlen(protocol) + 3 + strlen(host) + 1 + strlen(port) + strlen("/onvif/device_service") + 1];
    strcpy(fullurl,protocol);
    strcat(fullurl,"://");
    strcat(fullurl,host);
    strcat(fullurl,":");
    strcat(fullurl,port);
    strcat(fullurl,"/onvif/device_service");

    C_INFO("Manually adding URL : '%s'",fullurl);

    OnvifDevice * onvif_dev = OnvifDevice__create((char *)fullurl);
    if(!OnvifDevice__is_valid(onvif_dev)){
        C_ERROR("Invalid URL provided\n");
        OnvifDevice__destroy(onvif_dev);
        goto exit;
    }
    OnvifDevice__set_credentials(onvif_dev,AddDeviceDialog__get_user((AddDeviceDialog *)event->dialog),AddDeviceDialog__get_pass((AddDeviceDialog *)event->dialog));
   
    /* Authentication check */
    OnvifDevice__authenticate(onvif_dev);

    OnvifErrorTypes oerror = OnvifDevice__get_last_error(onvif_dev);
    if(oerror == ONVIF_ERROR_NONE) {
        //Extract scope
        OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_dev);
        OnvifScopes * scopes = OnvifDeviceService__getScopes(devserv);

        char * name = OnvifScopes__extract_scope(scopes,"name");
        char * hardware = OnvifScopes__extract_scope(scopes,"hardware");
        char * location = OnvifScopes__extract_scope(scopes,"location");

        GtkWidget * omgr_device = OnvifMgrDeviceRow__new((OnvifApp *) event->user_data, onvif_dev,name,hardware,location);
        
        gdk_threads_add_idle(G_SOURCE_FUNC(idle_add_device),omgr_device);

        free(name);
        free(hardware);
        free(location);
        //TODO Save manually added camera to settings

        OnvifScopes__destroy(scopes);
    } else {
        if(oerror == ONVIF_ERROR_NOT_AUTHORIZED){
            AddDeviceDialog__set_error(dialog,"Unauthorized...");
        } else if(oerror == ONVIF_ERROR_NOT_VALID){
            AddDeviceDialog__set_error(dialog,"Not a valid ONVIF device...");
        } else if(oerror == ONVIF_ERROR_SOAP){
            AddDeviceDialog__set_error(dialog,"Unexected soap error occured...");
        } else if(oerror == ONVIF_ERROR_CONNECTION){
            AddDeviceDialog__set_error(dialog,"Failed to connect...");
        }
        C_ERROR("An soap error was encountered %d\n",oerror);
        OnvifDevice__destroy(onvif_dev);
        goto exit;
    }

    gdk_threads_add_idle(G_SOURCE_FUNC(idle_hide_dialog),dialog);
exit:
    msgdialog = OnvifApp__get_msg_dialog((OnvifApp *) event->user_data);
    gdk_threads_add_idle(G_SOURCE_FUNC(idle_hide_dialog),msgdialog);
    free(event);
}

void add_device_add_cb(AppDialogEvent * event){
    OnvifApp * app = (OnvifApp *) event->user_data;
    const char * host = AddDeviceDialog__get_host((AddDeviceDialog *)event->dialog);
    if(!host || strlen(host) < 2){
        C_WARN("Host field invalid. (too short)");
        return;
    }

    GtkWidget * image = gtk_dotted_slider_new(GTK_ORIENTATION_HORIZONTAL, 5,10,1);
    MsgDialog * dialog = OnvifApp__get_msg_dialog(app);
    MsgDialog__set_icon(dialog, image);
    AppDialog__set_closable((AppDialog*)dialog, 0);
    AppDialog__hide_actions((AppDialog*)dialog);
    AppDialog__set_title((AppDialog*)dialog,"Working...");
    AppDialog__set_cancellable((AppDialog*)dialog,0);
    MsgDialog__set_message(dialog,"Testing ONVIF device configuration...");
    AppDialog__show((AppDialog *) dialog,NULL,NULL,NULL);

    EventQueue__insert(app->queue,_onvif_device_add,AppDialogEvent_copy(event));
}

void onvif_add_device (GtkWidget *widget, OnvifApp * app) {
    AppDialog__show((AppDialog *) app->add_dialog,add_device_add_cb,NULL,app);
}

/*
 *
 *  UI Creation
 *
 */
void create_ui (OnvifApp * app) {
    GtkWidget *main_window;  /* The uppermost window, containing all other windows */
    GtkWidget *grid;
    GtkWidget *widget;
    GtkWidget *label;
    GtkWidget * hbox;

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), app);

    gtk_window_set_title (GTK_WINDOW (main_window), "Onvif Device Manager");

    GtkWidget * overlay =gtk_overlay_new();
    gtk_container_add (GTK_CONTAINER (main_window), overlay);
    
    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new ();
    gtk_container_set_border_width (GTK_CONTAINER (grid), 10);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),grid);

    AppDialog__add_to_overlay((AppDialog *) app->profiles_dialog,GTK_OVERLAY(overlay));
    AppDialog__add_to_overlay((AppDialog *) app->add_dialog,GTK_OVERLAY(overlay));
    AppDialog__add_to_overlay((AppDialog *) app->cred_dialog,GTK_OVERLAY(overlay));
    AppDialog__add_to_overlay((AppDialog *) app->msg_dialog, GTK_OVERLAY(overlay));
    GtkWidget * left_grid = gtk_grid_new ();

    app->btn_scan = gtk_button_new ();
    label = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\">Scan</span>...");
    gtk_container_add (GTK_CONTAINER (app->btn_scan), label);
    gtk_widget_set_vexpand (app->btn_scan, FALSE);
    gtk_widget_set_hexpand (app->btn_scan, FALSE);
    gtk_widget_set_size_request(app->btn_scan,-1,80);
    gtk_grid_attach (GTK_GRID (left_grid), app->btn_scan, 0, 0, 1, 1);
    g_signal_connect (app->btn_scan, "clicked", G_CALLBACK (onvif_scan), app);

    widget = gtk_button_new ();
    label = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (label), "<span size=\"small\">Add...</span>...");
    gtk_container_add (GTK_CONTAINER (widget), label);
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_widget_set_hexpand (widget, FALSE);
    gtk_widget_set_size_request(widget,-1,5);
    gtk_widget_set_size_request(label,-1,5);
    gtk_grid_attach (GTK_GRID (left_grid), widget, 0, 1, 1, 1);
    g_signal_connect (widget, "clicked", G_CALLBACK (onvif_add_device), app);

    /* --- Create a list item from the data element --- */
    widget = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (widget, TRUE);
    app->listbox = gtk_list_box_new ();
    gtk_widget_set_vexpand (app->listbox, TRUE);
    gtk_widget_set_hexpand (app->listbox, FALSE);
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (app->listbox), GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(widget),app->listbox);
    gtk_grid_attach (GTK_GRID (left_grid), widget, 0, 2, 1, 1);
    g_signal_connect (app->listbox, "row-selected", G_CALLBACK (row_selected_cb), app);

    widget = gtk_button_new_with_label ("Quit");
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_widget_set_hexpand (widget, FALSE);
    g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK (quit_cb), app);
    gtk_grid_attach (GTK_GRID (left_grid), widget, 0, 3, 1, 1);


    GtkWidget *hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand (hpaned, TRUE);
    gtk_widget_set_hexpand (hpaned, TRUE);

    GtkStyleContext * context = gtk_widget_get_style_context (hpaned);
    GtkCssProvider * css = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (css,
                                    "paned separator{"
                                    //    "background-size:4px;"
                                    "min-width: 10px;"
                                    "}",
                                    -1,NULL);

    gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(css), 
                        GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_paned_pack1 (GTK_PANED (hpaned), left_grid, FALSE, FALSE);

    /* Create a new notebook, place the position of the tabs */
    app->main_notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (app->main_notebook), GTK_POS_TOP);
    gtk_widget_set_vexpand (app->main_notebook, TRUE);
    gtk_widget_set_hexpand (app->main_notebook, TRUE);
    gtk_paned_pack2 (GTK_PANED (hpaned), app->main_notebook, FALSE, FALSE);

    gtk_grid_attach (GTK_GRID (grid), hpaned, 0, 0, 1, 4);
    gtk_paned_set_wide_handle(GTK_PANED(hpaned),TRUE);
    char * TITLE_STR = "NVT";
    label = gtk_label_new (TITLE_STR);

    //Hidden spinner used to display stream start loading
    app->player_loading_handle = gtk_spinner_new ();

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),app->player_loading_handle,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = OnvifNVT__create_ui(app->player);
    gtk_notebook_append_page (GTK_NOTEBOOK (app->main_notebook), widget, hbox);

    label = gtk_label_new ("Details");
    //Hidden spinner used to display stream start loading
    widget = gtk_spinner_new ();
    OnvifDetails__set_details_loading_handle(app->details,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = OnvifDetails__get_widget(app->details);

    gtk_notebook_append_page (GTK_NOTEBOOK (app->main_notebook), widget, hbox);


    label = gtk_label_new ("Settings");
    //Hidden spinner used to display stream start loading
    widget = gtk_spinner_new ();
    AppSettings__set_details_loading_handle(app->settings,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = AppSettings__get_widget(app->settings);

    gtk_notebook_append_page (GTK_NOTEBOOK (app->main_notebook), widget, hbox);



    label = gtk_label_new ("Task Manager");
    //Hidden spinner used to display stream start loading
    // widget = gtk_spinner_new ();
    // TaskMgr__set_loading_handle(app->taskmgr,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);

    int running = EventQueue__get_running_event_count(app->queue);
    int pending = EventQueue__get_pending_event_count(app->queue);
    int total = EventQueue__get_thread_count(app->queue);
    char str[7];
    sprintf(str, "[%d/%d]", running + pending,total);
    app->task_label = gtk_label_new (str);
    gtk_box_pack_start(GTK_BOX(hbox),app->task_label,FALSE,FALSE,0);
    // gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show_all(hbox);

    widget = TaskMgr__get_widget(app->taskmgr);

    gtk_notebook_append_page (GTK_NOTEBOOK (app->main_notebook), widget, hbox);


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

    app->window = main_window;
    gtk_widget_show_all (main_window);
    g_signal_connect (G_OBJECT (app->main_notebook), "switch-page", G_CALLBACK (switch_page), app);

}

void _overscale_cb(AppSettingsStream * settings, int allow_overscale, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    RtspPlayer__allow_overscale(app->player,allow_overscale);
}

struct GUILabelUpdate {
    GtkWidget * label;
    char * text;
};

gboolean * gui_set_task_label (void * user_data){
    struct GUILabelUpdate * update = (struct  GUILabelUpdate *) user_data;
    gtk_label_set_text(GTK_LABEL(update->label),update->text);

    free(update->text);
    free(update);

    return FALSE;
}

void eventqueue_dispatch_cb(QueueThread * thread, EventQueueType type, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    if(!GTK_IS_WIDGET(app->task_label)){
        return;
    }
    EventQueue * queue = QueueThread__get_queue(thread);
    int running = EventQueue__get_running_event_count(queue);
    int pending = EventQueue__get_pending_event_count(queue);
    int total = EventQueue__get_thread_count(queue);

    char str[10];
    memset(&str,'\0',sizeof(str));
    sprintf(str, "[%d/%d]", running + pending,total);
    struct GUILabelUpdate * update = malloc(sizeof(struct GUILabelUpdate));
    update->label = app->task_label;

    update->text = malloc(strlen(str)+1);
    strcpy(update->text,str);
    gdk_threads_add_idle(G_SOURCE_FUNC(gui_set_task_label),update);
}

void OnvifApp__profile_selected(ProfilesDialog * dialog, OnvifProfile * profile){
    OnvifMgrDeviceRow * device =  ProfilesDialog__get_device(dialog);
    OnvifMgrDeviceRow__set_profile(device,profile);
}

OnvifApp * OnvifApp__create(){
    OnvifApp *app  =  malloc(sizeof(OnvifApp));
    app->device = NULL;
    app->task_label = NULL;
    app->queue = EventQueue__create(eventqueue_dispatch_cb,app);
    app->details = OnvifDetails__create(app);
    app->settings = AppSettings__create(app->queue);
    app->taskmgr = TaskMgr__create();

    AppSettingsStream__set_overscale_callback(app->settings->stream,_overscale_cb,app);
    app->current_page = 0;
    app->player = RtspPlayer__create();
    RtspPlayer__allow_overscale(app->player,AppSettingsStream__get_allow_overscale(app->settings->stream));

    //Defaults 8 paralell event threads.
    //TODO support configuration to modify this
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);
    EventQueue__start(app->queue);

    RtspPlayer__set_retry_callback(app->player, retry_onvif_stream, app);
    RtspPlayer__set_error_callback(app->player, error_onvif_stream, app);
    RtspPlayer__set_stopped_callback(app->player, stopped_onvif_stream, app);
    RtspPlayer__set_started_callback(app->player, started_onvif_stream, app);

    app->profiles_dialog = ProfilesDialog__create(app->queue, OnvifApp__profile_selected);
    app->add_dialog = AddDeviceDialog__create();
    app->cred_dialog = CredentialsDialog__create();
    app->msg_dialog = MsgDialog__create();
    
    create_ui (app);

    return app;
}

void OnvifApp__destroy(OnvifApp* self){
    if (self) {
        OnvifDetails__destroy(self->details);
        AppSettings__destroy(self->settings);
        TaskMgr__destroy(self->taskmgr);
        CObject__destroy((CObject*)self->profiles_dialog);
        CObject__destroy((CObject*)self->add_dialog);
        CObject__destroy((CObject*)self->cred_dialog);
        CObject__destroy((CObject*)self->msg_dialog);
        //Destroying the queue will hang until all threads are stopped
        CObject__destroy((CObject*)self->queue);
        //Destroying the player will cause it to hang until its state changed to NULL
        //Destroying the player after the queue because the queue could dispatch retry call
        RtspPlayer__destroy(self->player);
        
        //Using idle destruction to allow pending idles to execute
        safely_destroy_widget(self->window);

        free(self);
    }
}
void _profile_callback (void * user_data){
    C_TRACE("_profile_callback");
    OnvifMgrDeviceRow * device = ONVIFMGR_DEVICEROW(user_data);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)){
        C_TRAIL("_profile_callback - invalid device.");
        return;
    }
    
    if(OnvifMgrDeviceRow__is_selected(device)){
        RtspPlayer__stop(OnvifMgrDeviceRow__get_app(device)->player);
    }
    onvif_reload_device(device);

    g_object_unref(device);
}

static void show_profiles_dialog (OnvifMgrDeviceRow *device){
    ProfilesDialog__set_device(OnvifMgrDeviceRow__get_app(device)->profiles_dialog,device);
    AppDialog__show((AppDialog *) OnvifMgrDeviceRow__get_app(device)->profiles_dialog,NULL,NULL,NULL);
}

void add_device(OnvifMgrDeviceRow * omgr_device){
    g_signal_connect (G_OBJECT (omgr_device), "profile-clicked", G_CALLBACK (show_profiles_dialog), NULL);

    OnvifApp * app = OnvifMgrDeviceRow__get_app(omgr_device);
    gtk_list_box_insert (GTK_LIST_BOX (app->listbox), GTK_WIDGET(omgr_device), -1);
    gtk_widget_show_all (GTK_WIDGET(omgr_device));

    onvif_display_device_row(app,omgr_device);
}

RtspPlayer * OnvifApp__get_player(OnvifApp* self){
    return self->player;
}

MsgDialog * OnvifApp__get_msg_dialog(OnvifApp * self){
    return self->msg_dialog;
}

ProfilesDialog * OnvifApp__get_profiles_dialog(OnvifApp * self){
    return self->profiles_dialog;
}

OnvifMgrDeviceRow * OnvifApp__get_device(OnvifApp * self){
    return self->device;
}

gboolean OnvifApp__set_device(OnvifApp * app, GtkListBoxRow * row){
    if(!ONVIFMGR_IS_DEVICEROW(row)) {
        C_DEBUG("OnvifApp__set_device - NULL");
        if(ONVIFMGR_IS_DEVICEROW(app->device)) g_object_unref(app->device);
        app->device = NULL;
        return FALSE; 
    }

    OnvifMgrDeviceRow * old = app->device;

    C_DEBUG("OnvifApp__set_device - valid device");
    app->device = ONVIFMGR_DEVICEROW(row);

    //If changing device, trade reference
    if(ONVIFMGR_DEVICEROW(row) != app->device) {
        g_object_unref(old);
        g_object_ref(app->device);
    }

    return TRUE;
}

void OnvifApp__dispatch(OnvifApp* app, void (*callback)(), void * user_data){
    EventQueue__insert(app->queue,callback,user_data);
}