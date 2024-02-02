#include "onvif_app.h"
#include "dialog/credentials_input.h"
#include "dialog/add_device.h"
#include "onvif_details.h"
#include "onvif_nvt.h"
#include "device.h"
#include "gui_utils.h"
#include "settings/app_settings.h"
#include "../queue/event_queue.h"
#include "discoverer.h"
#include "task_manager.h"
#include "clist_ts.h"
#include "clogger.h"
#include "../animations/dotted_slider.h"

typedef struct _OnvifApp {
    Device* device; /* Currently selected device */
    CListTS *device_list;

    GtkWidget *window;
    GtkWidget *listbox;
    GtkWidget *main_notebook;
    GtkWidget *player_loading_handle;
    GtkWidget *task_label;

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

struct DeviceInput {
    Device * device;
    OnvifApp * app;
    int skip_profiles;
};

struct DiscoveryInput {
    OnvifApp * app;
    GtkWidget * widget;
};

typedef struct {
    OnvifApp * app;
    OnvifDevice * device;
    OnvifScopes * scopes;
} GUIScopeInput;

typedef struct {
    OnvifApp * app;
    OnvifDevice * device;
} OnvifDeviceInput;

void stopped_onvif_stream(RtspPlayer * player, void * user_data);

struct DeviceInput * DeviceInput__copy(struct DeviceInput * self){
    struct DeviceInput * copy = malloc(sizeof(struct DeviceInput));
    copy->app = self->app;
    copy->device = self->device;
    copy->skip_profiles = self->skip_profiles;
    return copy;
}

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

void add_device(OnvifApp * self, OnvifDevice * onvif_dev, char* name, char * hardware, char * location);

static int device_already_found(OnvifApp * app, char * xaddr){
    int b;
    int ret = 0;
    for (b=0;b<CListTS__get_count(app->device_list);b++){
        Device * dev = (Device *) CListTS__get(app->device_list,b);
        OnvifDevice * odev = Device__get_device(dev);
        OnvifDeviceService * devserv = OnvifDevice__get_device_service(odev);
        char * endpoint = OnvifDeviceService__get_endpoint(devserv);
        if(!strcmp(xaddr,endpoint)){
            C_DEBUG("Record already part of list [%s]\n",endpoint);
            ret = 1;
        }
        free(endpoint);
    }
    return ret;
}

static gboolean * found_server (void * e) {
    C_TRACE("found_server");
    DiscoveryEvent * event = (DiscoveryEvent *) e;

    DiscoveredServer * server = event->server;
    struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;

    ProbMatch * m;
    int i;
  
    C_INFO("Found server - Match count : %i\n",server->matches->match_count);
    for (i = 0 ; i < server->matches->match_count ; ++i) {
        m = server->matches->matches[i];
        if(!device_already_found(disco_in->app,m->addrs[0])){
            gchar * addr_dup = g_strdup(m->addrs[0]);
            OnvifDevice * onvif_dev = OnvifDevice__create(addr_dup);
            free(addr_dup);
            
            char * name = onvif_extract_scope("name",m);
            char * hw = onvif_extract_scope("hardware",m);
            char * loc = onvif_extract_scope("location",m);
            add_device(disco_in->app, onvif_dev, name, hw, loc);
            free(name);
            free(hw);
            free(loc);
        }
    }

    CObject__destroy((CObject*)event);
    return FALSE;
}

void _found_server (DiscoveryEvent * event) {
    C_TRACE("_found_server");
    gdk_threads_add_idle((void *)found_server,event);
}

gboolean * finished_discovery (void * e) {
    C_TRACE("finished_discovery");
    struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) e;

    gtk_widget_set_sensitive(disco_in->widget,TRUE);

    free(disco_in);

    return FALSE;
}

void _start_onvif_discovery(void * user_data){
    struct DiscoveryInput * disco_in = (struct DiscoveryInput *) user_data;
    //Start UDP Scan
    struct UdpDiscoverer discoverer = UdpDiscoverer__create(_found_server);
    UdpDiscoverer__start(&discoverer, disco_in, AppSettingsDiscovery__get_repeat(disco_in->app->settings->discovery), AppSettingsDiscovery__get_timeout(disco_in->app->settings->discovery));
    
    gdk_threads_add_idle((void *)finished_discovery,disco_in);
}

void onvif_scan (GtkWidget *widget, OnvifApp * app) {
    C_INFO("Starting ONVIF Devices Network Discovery...");
    gtk_widget_set_sensitive(widget,FALSE);
    //Between retries, player may not send stopped event since the state didn't stop
    //Forcing hiding the loading indicator
    stopped_onvif_stream(app->player,app);

    //Clearing the list
    gtk_container_foreach (GTK_CONTAINER (app->listbox), (GtkCallback)gtk_widget_destroy, NULL);
    EventQueue__clear(app->queue);
    CListTS__clear(app->device_list);

    //Multiple dispatch in case of packet dropped
    struct DiscoveryInput * disco_in = malloc(sizeof(struct DiscoveryInput));
    disco_in->app = app;
    disco_in->widget = widget;

    EventQueue__insert(app->queue,_start_onvif_discovery,disco_in);
}

void error_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    gtk_spinner_stop (GTK_SPINNER (app->player_loading_handle));
}

void stopped_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    gtk_spinner_stop (GTK_SPINNER (app->player_loading_handle));
}

void start_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    gtk_spinner_start (GTK_SPINNER (app->player_loading_handle));
}

void _retry_onvif_stream(void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    sleep(1);
    RtspPlayer__retry(app->player);
}

void retry_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    EventQueue__insert(app->queue,_retry_onvif_stream,app);
}

void _stop_onvif_stream(void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    RtspPlayer__stop(app->player);
}

void _display_onvif_device(void * user_data){
    struct DeviceInput * input = (struct DeviceInput *) user_data;
    if(!CObject__addref((CObject*)input->device)){
        free(input);
        return;
    }
    
    OnvifDevice * odev = Device__get_device(input->device);

    /* Start by authenticating the device then start retrieve thumbnail */
    if(OnvifDevice__get_last_error(odev) != ONVIF_ERROR_NONE)
        OnvifDevice__authenticate(odev);

    if(!CObject__is_valid((CObject*)input->device)){
        goto exit;
    }

    /* Display Profile dropdown */
    if(!input->skip_profiles && OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NONE)
        Device__load_profiles(input->device,input->app->queue);

    /* Display row thumbnail. Default to profile index 0 */
    Device__load_thumbnail(input->device,input->app->queue);

exit:
    CObject__unref((CObject*)input->device);
    free(input);
}

void onvif_display_device_row(OnvifApp * self, Device * device, int skip_profiles){
    struct DeviceInput * input = malloc(sizeof(struct DeviceInput));
    input->device = device;
    input->app = self;
    input->skip_profiles = skip_profiles;

    /* nslookup doesn't require onvif authentication. Dispatch event now. */
    Device__lookup_hostname(device,self->queue);

    EventQueue__insert(self->queue,_display_onvif_device,input);
}

void _play_onvif_stream(void * user_data){
    struct DeviceInput * input = (struct DeviceInput *) user_data;
    C_DEBUG("_play_onvif_stream\n");
    //Check if device is still valid. (User performed scan before thread started)
    if(!CObject__addref((CObject*)input->device)){
        free(input);
        return;
    }

    if(!Device__is_selected(input->device)){
        goto exit;
    }

    OnvifDevice * odev = Device__get_device(input->device);

    //Display loading while StreamURI is fetched
    start_onvif_stream(input->app->player,input->app);

    if(OnvifDevice__get_last_error(odev) != ONVIF_ERROR_NONE)
        OnvifDevice__authenticate(odev);

    if(OnvifDevice__get_last_error(odev) != ONVIF_ERROR_NONE && Device__is_selected(input->device)){
        stopped_onvif_stream(input->app->player,input->app);
        goto exit;
    }

    /* Set the URI to play */
    OnvifMediaService * media_service = OnvifDevice__get_media_service(odev);
    char * uri = OnvifMediaService__getStreamUri(media_service,Device__get_selected_profile(input->device));
    if(!uri){
        stopped_onvif_stream(input->app->player,input->app);
        goto exit;
    }

    if(CObject__is_valid((CObject*)input->device) && OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NONE){
        RtspPlayer__set_playback_url(input->app->player,uri);
        char * port = OnvifDevice__get_port(Device__get_device(input->device));
        RtspPlayer__set_port_fallback(input->app->player,port);
        free(port);
        OnvifCredentials * ocreds = OnvifDevice__get_credentials(odev);
        RtspPlayer__set_credentials(input->app->player, OnvifCredentials__get_username(ocreds), OnvifCredentials__get_password(ocreds));
    }
    free(uri);

    //User performed scan before StreamURI was retrieved
    if(!CObject__is_valid((CObject*)input->device) || OnvifDevice__get_last_error(odev) != ONVIF_ERROR_NONE){
        goto exit;
    }

    if(!Device__is_selected(input->device)){
        goto exit;
    }

    RtspPlayer__play(input->app->player);

exit:
    C_TRACE("_play_onvif_stream - done\n");
    CObject__unref((CObject*)input->device);
    free(input);
}


void update_pages(OnvifApp * self){
    //#0 NVT page already loaded
    //#2 Settings page already loaded
    if(!CObject__addref((CObject*)self->device)){
        return;
    }

    //Details page is dynamically loaded
    if(self->current_page == 1){
        OnvifDetails__clear_details(self->details);
        OnvifDetails__update_details(self->details,self->device);
    }

    CObject__unref((CObject*)self->device);
}

gboolean * gui_update_pages(void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    AppDialog__hide((AppDialog*)app->cred_dialog);
    update_pages(app);
    return FALSE;
}

int onvif_reload_device(struct DeviceInput * input){
    OnvifDevice * odev = Device__get_device(input->device);

    if(OnvifDevice__get_last_error(odev) != ONVIF_ERROR_NONE)
        OnvifDevice__authenticate(odev);
    //Check if device is valid and authorized (User performed scan before auth finished)
    if(!CObject__is_valid((CObject*)input->device) || OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NOT_AUTHORIZED){
        return 0;
    }

    //Replace locked image with spinner
    GtkWidget * image = gtk_spinner_new ();
    Device__set_thumbnail(input->device,image);

    onvif_display_device_row(input->app, input->device, input->skip_profiles);
    gdk_threads_add_idle((void *)gui_update_pages,input->app);
    if(OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NONE)
        _play_onvif_stream(DeviceInput__copy(input));

    return 1;
}

void _onvif_authentication_reload(void * user_data){
    AppDialogEvent * event = (AppDialogEvent *) user_data;
    struct DeviceInput * input = (struct DeviceInput *) event->user_data;
    Device * device = input->device;
    C_DEBUG("_onvif_authentication_reload\n");
    //Check device is still valid before adding ref (User performed scan before thread started)
    if(!CObject__addref((CObject*)device)){
        C_WARN("_onvif_authentication_reload - invalid device\n");
        free(event);
        return;
    }

    if(onvif_reload_device(input)){
        free(input);//Successful login, dialog is gone, input is no longer needed.
    }

    CObject__unref((CObject*)device);
    free(event);
}

void dialog_login_cb(AppDialogEvent * event){
    C_INFO("OnvifAuthentication attempt...\n");
    struct DeviceInput * input = (struct DeviceInput *) event->user_data;
    OnvifDevice__set_credentials(Device__get_device(input->device),CredentialsDialog__get_username((CredentialsDialog*)event->dialog),CredentialsDialog__get_password((CredentialsDialog*)event->dialog));
    EventQueue__insert(input->app->queue,_onvif_authentication_reload,AppDialogEvent_copy(event));
}

void dialog_cancel_cb(AppDialogEvent * event){
    C_INFO("OnvifAuthentication cancelled...\n");
    free(event->user_data);
}

gboolean * gui_show_credentialsdialog (void * user_data){
    struct DeviceInput * input = (struct DeviceInput *) user_data;
    AppDialog__show((AppDialog *)input->app->cred_dialog,dialog_login_cb, dialog_cancel_cb,input);
    return FALSE;
}

void OnvifApp__select_device(OnvifApp * app,  GtkListBoxRow * row){
    struct DeviceInput input;
    memset(&input, 0, sizeof(input));
    input.app = app;

    C_INFO("OnvifApp__select_device\n");
    
    //Stop previous stream
    EventQueue__insert(app->queue,_stop_onvif_stream,app);

    //Clear details
    OnvifDetails__clear_details(app->details);

    if(input.app->device){
        Device__set_selected(input.app->device,0);
    }

    if(row == NULL){
        input.app->device = NULL;
        return;
    }

    //Set newly selected device
    int pos;
    pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
    input.app->device = (Device *) CListTS__get(input.app->device_list,pos);;
    Device__set_selected(input.app->device,1);
    input.device = input.app->device;
    
    OnvifDevice * odev = Device__get_device(input.device);
    //Prompt for authentication
    if(OnvifDevice__get_last_error(odev) == ONVIF_ERROR_NOT_AUTHORIZED){
        input.skip_profiles = 0;
        //Re-dispatched to allow proper focus handling
        gdk_threads_add_idle((void *)gui_show_credentialsdialog,DeviceInput__copy(&input));
        return;
    }

    EventQueue__insert(app->queue,_play_onvif_stream,DeviceInput__copy(&input));
    update_pages(input.app);
}

void row_selected_cb (GtkWidget *widget,   GtkListBoxRow* row, OnvifApp* app){
    OnvifApp__select_device(app,row);
}

static void switch_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifApp * app) {
    app->current_page = page_num;
    update_pages(app);
}

gboolean gui_process_device_scopes(void * user_data){
    GUIScopeInput * input = (GUIScopeInput *) user_data;
    char * name = OnvifScopes__extract_scope(input->scopes,"name");
    char * hardware = OnvifScopes__extract_scope(input->scopes,"hardware");
    char * location = OnvifScopes__extract_scope(input->scopes,"location");
    add_device(input->app, input->device, name, hardware, location);
    free(name);
    free(hardware);
    free(location);
    //TODO Save manually added camera to settings

    OnvifScopes__destroy(input->scopes);
    free(input);
    return FALSE;
}

gboolean * gui_hide_dialog (void * user_data){
    AppDialog * dialog = (AppDialog *) user_data;
    AppDialog__hide(dialog);
    return FALSE;
}


void _onvif_device_add(void * user_data){
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
   

    OnvifDevice__authenticate(onvif_dev);
    OnvifErrorTypes oerror = OnvifDevice__get_last_error(onvif_dev);
    if(oerror == ONVIF_ERROR_NONE) {
        //Extract scope
        OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_dev);
        OnvifScopes * scopes = OnvifDeviceService__getScopes(devserv);
        GUIScopeInput * guiscope = malloc(sizeof(GUIScopeInput));
        guiscope->app = (OnvifApp *) event->user_data;
        guiscope->device = onvif_dev;
        guiscope->scopes = scopes;
        gdk_threads_add_idle((void *)gui_process_device_scopes,guiscope);
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

    gdk_threads_add_idle((void *)gui_hide_dialog,dialog);
exit:
    MsgDialog * msgdialog = OnvifApp__get_msg_dialog((OnvifApp *) event->user_data);
    gdk_threads_add_idle((void *)gui_hide_dialog,msgdialog);
    free(event);
}

void add_device_add_cb(AppDialogEvent * event){
    OnvifApp * app = (OnvifApp *) event->user_data;
    const char * host = AddDeviceDialog__get_host((AddDeviceDialog *)event->dialog);
    if(!host || strlen(host) < 2){
        C_WARN("Host field invalid. (too short)");
        return;
    }

    GtkWidget * image = create_dotted_slider_animation(10,1);
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

    AppDialog__add_to_overlay((AppDialog *) app->add_dialog,GTK_OVERLAY(overlay));
    AppDialog__add_to_overlay((AppDialog *) app->cred_dialog,GTK_OVERLAY(overlay));
    AppDialog__add_to_overlay((AppDialog *) app->msg_dialog, GTK_OVERLAY(overlay));
    GtkWidget * left_grid = gtk_grid_new ();

    widget = gtk_button_new ();
    label = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\">Scan</span>...");
    gtk_container_add (GTK_CONTAINER (widget), label);
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_widget_set_hexpand (widget, FALSE);
    gtk_widget_set_size_request(widget,-1,80);
    gtk_grid_attach (GTK_GRID (left_grid), widget, 0, 0, 1, 1);
    g_signal_connect (widget, "clicked", G_CALLBACK (onvif_scan), app);

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
    gdk_threads_add_idle((void *)gui_set_task_label,update);
}

OnvifApp * OnvifApp__create(){
    OnvifApp *app  =  malloc(sizeof(OnvifApp));
    app->device_list = CListTS__create();
    app->device = NULL;
    app->task_label = NULL;
    app->add_dialog = AddDeviceDialog__create();
    app->cred_dialog = CredentialsDialog__create();
    app->msg_dialog = MsgDialog__create();
    app->queue = EventQueue__create(eventqueue_dispatch_cb,app);
    app->details = OnvifDetails__create(app->queue);
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
    RtspPlayer__set_start_callback(app->player, start_onvif_stream, app);
    
    create_ui (app);

    return app;
}

void OnvifApp__destroy(OnvifApp* self){
    if (self) {
        OnvifDetails__destroy(self->details);
        AppSettings__destroy(self->settings);
        TaskMgr__destroy(self->taskmgr);
        CObject__destroy((CObject*)self->add_dialog);
        CObject__destroy((CObject*)self->cred_dialog);
        CObject__destroy((CObject*)self->msg_dialog);
        //Destroying the player will cause it to hang until its state changed to NULL
        RtspPlayer__destroy(self->player);
        //Destroying the queue will hang until all threads are stopped
        CObject__destroy((CObject*)self->queue);
        CObject__destroy((CObject *)self->device_list);
        safely_destroy_widget(self->window);
        free(self);
    }
}

void profile_callback(Device * device, void * user_data){
    C_INFO("Switching profile");
    struct DeviceInput input;
    input.app = (OnvifApp *) user_data;
    input.device = device;
    input.skip_profiles = 1;
    RtspPlayer__stop(input.app->player);
    onvif_reload_device(&input);
}

void add_device(OnvifApp * self, OnvifDevice * onvif_dev, char* name, char * hardware, char * location){
    Device * device = Device__create(onvif_dev);
    Device__set_profile_callback(device,profile_callback,self);

    CListTS__add(self->device_list,(CObject*)device);
    int b;
    for (b=0;b<CListTS__get_count(self->device_list);b++){
        Device * dev = (Device *) CListTS__get(self->device_list,b);
        OnvifDevice * odev = Device__get_device(dev);
        char * dev_host = OnvifDevice__get_host(odev);
        char * dev_port = OnvifDevice__get_port(odev);
        C_DEBUG("\tList Record :[%i] %s:%s\n",b,dev_host,dev_port);
        free(dev_host);
        free(dev_port);
    }

    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_dev);
    char * endpoint = OnvifDeviceService__get_endpoint(devserv);

    GtkWidget * row = Device__create_row(device, endpoint, name, hardware, location);
    free(endpoint);

    gtk_list_box_insert (GTK_LIST_BOX (self->listbox), row, -1);
    gtk_widget_show_all (row);

    onvif_display_device_row(self,device, 0);
}

RtspPlayer * OnvifApp__get_player(OnvifApp* self){
    return self->player;
}

MsgDialog * OnvifApp__get_msg_dialog(OnvifApp * self){
    return self->msg_dialog;
}