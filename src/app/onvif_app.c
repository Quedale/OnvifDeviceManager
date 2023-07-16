#include "onvif_app.h"
#include "credentials_input.h"
#include "add_device.h"
#include "onvif_details.h"
#include "onvif_nvt.h"
#include "device.h"
#include "gui_utils.h"
#include "settings/app_settings.h"
#include "../queue/event_queue.h"
#include "../gst/player.h"
#include "discoverer.h"
#include "task_manager.h"
#include "../oo/clist_ts.h"

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

gboolean * gui_destruction (void * user_data){
    OnvifApp *data = (OnvifApp *) user_data;
    
    OnvifApp__destroy(data);
    gtk_main_quit();

    return FALSE;
}

/* This function is called when the STOP button is clicked */
static void quit_cb (GtkButton *button, OnvifApp *data) {
    RtspPlayer__stop(data->player);
    gtk_main_quit();

    /*
    * Before thread cleanup can be done, gui showin the progress should be created
    * This is to prevent hidden dangling process
    */
    // gtk_widget_hide(data->window);
    // //Dispatch a new event to allow the window to hide.
    // gdk_threads_add_idle((void *)gui_destruction,data);
}

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, OnvifApp *data) {
    RtspPlayer__stop(data->player);
    gtk_main_quit ();
}

static gboolean * finished_discovery (void * e) {
    DiscoveryEvent * event = (DiscoveryEvent *) e;
    struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;
    
    gtk_widget_set_sensitive(disco_in->widget,TRUE);
    free(disco_in);
    return FALSE;
}

void add_device(OnvifApp * self, OnvifDevice * onvif_dev, char* name, char * hardware, char * location);

static int device_already_found(OnvifApp * app, char * xaddr){
    int b;
    for (b=0;b<app->device_list->count;b++){
        Device * dev = (Device *) app->device_list->data[b];
        OnvifDevice * odev = Device__get_device(dev);
        if(!strcmp(xaddr,odev->device_soap->endpoint)){
            printf("Record already part of list [%s]\n",odev->device_soap->endpoint);
            return 1;
        }
    }
    return 0;
}

static gboolean * found_server (void * e) {
    DiscoveryEvent * event = (DiscoveryEvent *) e;

    DiscoveredServer * server = event->server;
    struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;

    ProbMatch * m;
    int i;
  
    printf("Found server - Match count : %i\n",server->matches->match_count);
    for (i = 0 ; i < server->matches->match_count ; ++i) {
        m = server->matches->matches[i];
        if(!device_already_found(disco_in->app,m->addrs[0])){
            OnvifDevice * onvif_dev = OnvifDevice__create(g_strdup(m->addrs[0]));
            add_device(disco_in->app, onvif_dev, onvif_extract_scope("name",m), onvif_extract_scope("hardware",m), onvif_extract_scope("location",m));
        }
    }

    return FALSE;
}

void onvif_scan (GtkWidget *widget, OnvifApp * app) {

    gtk_widget_set_sensitive(widget,FALSE);

    //Clearing the list
    gtk_container_foreach (GTK_CONTAINER (app->listbox), (GtkCallback)gtk_widget_destroy, NULL);
    CListTS__clear(app->device_list);

    //Start UDP Scan
    struct UdpDiscoverer discoverer = UdpDiscoverer__create(found_server,finished_discovery);

    //Multiple dispatch in case of packet dropped
    struct DiscoveryInput * disco_in = malloc(sizeof(struct DiscoveryInput));
    disco_in->app = app;
    disco_in->widget = widget;
    //TODO Support retry_count by settings
    UdpDiscoverer__start(&discoverer, disco_in, 2);
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
    OnvifDevice * odev = Device__get_device(input->device);

    /* Start by authenticating the device then start retrieve thumbnail */
    if(odev->last_error != ONVIF_ERROR_NONE)
        OnvifDevice_authenticate(odev);

    /* Display Profile dropdown */
    if(!input->skip_profiles && odev->last_error == ONVIF_ERROR_NONE)
        Device__load_profiles(input->device,input->app->queue);

    /* Display row thumbnail. Default to profile index 0 */
    Device__load_thumbnail(input->device,input->app->queue);

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

    //Check if device is still valid. (User performed scan before thread started)
    if(!CObject__addref((CObject*)input->device)){
        goto exit;
    }

    if(!Device__is_selected(input->device)){
        return;
    }

    OnvifDevice * odev = Device__get_device(input->device);

    //Display loading while StreamURI is fetched
    start_onvif_stream(input->app->player,input->app);

    if(odev->last_error != ONVIF_ERROR_NONE)
        OnvifDevice_authenticate(odev);

        /* Set the URI to play */
        //TODO handle profiles
    if(odev->last_error != ONVIF_ERROR_NONE && Device__is_selected(input->device)){
        stopped_onvif_stream(input->app->player,input->app);
        goto exit;
    }

    char * uri = OnvifDevice__media_getStreamUri(odev,Device__get_selected_profile(input->device));
    if(!uri){
        stopped_onvif_stream(input->app->player,input->app);
        goto exit;
    }

    if(CObject__is_valid((CObject*)input->device) && odev->last_error == ONVIF_ERROR_NONE){
        RtspPlayer__set_playback_url(input->app->player,uri);
    }
    free(uri);

    //User performed scan before StreamURI was retrieved
    if(!CObject__is_valid((CObject*)input->device) || odev->last_error != ONVIF_ERROR_NONE){
        goto exit;
    }

    if(!Device__is_selected(input->device)){
        goto exit;
    }

    RtspPlayer__play(input->app->player);

exit:
    CObject__unref((CObject*)input->device);
    free(input);
}


void update_pages(OnvifApp * self){
    //#0 NVT page already loaded
    //#2 Settings page already loaded

    //Details page is dynamically loaded
    if(self->current_page == 1){
        OnvifDetails__clear_details(self->details);
        OnvifDetails__update_details(self->details,self->device);
    }
}

gboolean * gui_update_pages(void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    AppDialog__hide((AppDialog*)app->cred_dialog);
    update_pages(app);
    return FALSE;
}

int onvif_reload_device(struct DeviceInput * input){
    OnvifDevice * odev = Device__get_device(input->device);

    if(odev->last_error != ONVIF_ERROR_NONE)
        OnvifDevice_authenticate(odev);
    //Check if device is valid and authorized (User performed scan before auth finished)
    if(!CObject__is_valid((CObject*)input->device) || odev->last_error == ONVIF_NOT_AUTHORIZED){
        return 0;
    }

    //Replace locked image with spinner
    GtkWidget * image = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (image));
    Device__set_thumbnail(input->device,image);

    onvif_display_device_row(input->app, input->device, input->skip_profiles);
    if(odev->last_error == ONVIF_ERROR_NONE)
        EventQueue__insert(input->app->queue,_play_onvif_stream,DeviceInput__copy(input));
    gdk_threads_add_idle((void *)gui_update_pages,input->app);

    return 1;
}

void _onvif_authentication_reload(void * user_data){
    AppDialogEvent * event = (AppDialogEvent *) user_data;
    struct DeviceInput * input = (struct DeviceInput *) event->user_data;
    Device * device = input->device;
    //Check device is still valid before adding ref (User performed scan before thread started)
    if(!CObject__addref((CObject*)device)){
        goto exit;
    }

    OnvifDevice_set_credentials(Device__get_device(device),CredentialsDialog__get_username((CredentialsDialog*)event->dialog),CredentialsDialog__get_password((CredentialsDialog*)event->dialog));
    if(onvif_reload_device(input)){
        free(input);//Successful login, dialog is gone, input is no longer needed.
    }
exit:
    CObject__unref((CObject*)device);
    free(event);
}

void dialog_login_cb(AppDialogEvent * event){
    printf("OnvifAuthentication attempt...\n");
    struct DeviceInput * input = (struct DeviceInput *) event->user_data;
    EventQueue__insert(input->app->queue,_onvif_authentication_reload,AppDialogEvent_copy(event));
}

void dialog_cancel_cb(AppDialogEvent * event){
    printf("OnvifAuthentication cancelled...\n");
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
    input.app->device = (Device *) input.app->device_list->data[pos];
    Device__set_selected(input.app->device,1);
    input.device = input.app->device;
    
    OnvifDevice * odev = Device__get_device(input.device);
    //Prompt for authentication
    if(odev->last_error == ONVIF_NOT_AUTHORIZED){
        input.skip_profiles = 0;
        //Re-dispatched to allow proper focus handling
        gdk_threads_add_idle((void *)gui_show_credentialsdialog,DeviceInput__copy(&input));
        return;
    }

    EventQueue__insert(app->queue,_play_onvif_stream,DeviceInput__copy(&input));
    update_pages(input.app);
}

void row_selected_cb (GtkWidget *widget,   GtkListBoxRow* row,
  OnvifApp* app)
{
    OnvifApp__select_device(app,row);    
}

static void switch_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifApp * app) {
    app->current_page = page_num;
    update_pages(app);
}

void gui_process_device_scopes(void * user_data){
    GUIScopeInput * input = (GUIScopeInput *) user_data;
    add_device(input->app, input->device, OnvifScopes__extract_scope(input->scopes,"name"), OnvifScopes__extract_scope(input->scopes,"hardware"), OnvifScopes__extract_scope(input->scopes,"location"));
    //TODO Save manually added camera to settings

    OnvifScopes__destroy(input->scopes);
    free(input);
}

void _onvif_authentication_add(void * user_data){
    AppDialogEvent * event = (AppDialogEvent *) user_data;
    OnvifDeviceInput * input = (OnvifDeviceInput *) event->user_data;

    OnvifDevice_set_credentials(input->device,CredentialsDialog__get_username((CredentialsDialog*)event->dialog),CredentialsDialog__get_password((CredentialsDialog*)event->dialog));
    OnvifDevice_authenticate(input->device);
    if(input->device->last_error == ONVIF_NOT_AUTHORIZED){
        printf("Authentication failed\n");
        goto exit;
    }

    OnvifScopes * scopes = OnvifDevice__device_getScopes(input->device);

    GUIScopeInput * guiscope = malloc(sizeof(GUIScopeInput));
    guiscope->app = input->app;
    guiscope->device = input->device;
    guiscope->scopes = scopes;
    gdk_threads_add_idle((void *)gui_process_device_scopes,guiscope);

    //Hide both dialogs
    AppDialog__hide((AppDialog *)input->app->cred_dialog);
    AppDialog__hide((AppDialog *)input->app->add_dialog);

    //Input was used to dispatch credential dialog user_data.
    free(input);

exit:
    free(event);
}

void dialog_login_add_cb(AppDialogEvent * event){
    printf("OnvifAuthentication add attempt...\n");
    OnvifDeviceInput * input = (OnvifDeviceInput *) event->user_data;
    EventQueue__insert(input->app->queue,_onvif_authentication_add,AppDialogEvent_copy(event));
}

void dialog_cancel_add_cb(AppDialogEvent * event){
    AppDialog * dialog = (AppDialog*)event->dialog;
    OnvifDeviceInput * input = (OnvifDeviceInput *) dialog->user_data;
    OnvifDevice__destroy(input->device);
    dialog_cancel_cb(event);
}

gboolean * gui_show_credentialsdialog_add (void * user_data){
    OnvifDeviceInput * input = (OnvifDeviceInput *) user_data;
    AppDialog__show((AppDialog*)input->app->cred_dialog,dialog_login_add_cb, dialog_cancel_add_cb,input);
    return FALSE;
}

void _onvif_device_add(void * user_data){
    AppDialogEvent * event = (AppDialogEvent *) user_data;
    AddDeviceDialog * dialog = (AddDeviceDialog *) event->dialog;
    const char * device_uri = AddDeviceDialog__get_device_uri(dialog);

    OnvifDevice * onvif_dev = OnvifDevice__create(device_uri);
    if(!OnvifDevice__is_valid(onvif_dev)){
        printf("Invalid URL provided\n");
        OnvifDevice__destroy(onvif_dev);
        goto exit;
    } else {
        printf("Valid device added [%s]\n",device_uri);
    }

    OnvifDevice_authenticate(onvif_dev);
    if(onvif_dev->last_error == ONVIF_NOT_AUTHORIZED){
        OnvifDeviceInput * input = malloc(sizeof(OnvifDeviceInput));
        input->app = (OnvifApp *) event->user_data;
        input->device = onvif_dev;
        gdk_threads_add_idle((void *)gui_show_credentialsdialog_add,input);
    } else if(onvif_dev->last_error == ONVIF_ERROR_NONE) {
        //Extract scope
        OnvifScopes * scopes = OnvifDevice__device_getScopes(onvif_dev);
        GUIScopeInput * guiscope = malloc(sizeof(GUIScopeInput));
        guiscope->app = (OnvifApp *) event->user_data;
        guiscope->device = onvif_dev;
        guiscope->scopes = scopes;
        gdk_threads_add_idle((void *)gui_process_device_scopes,guiscope);
    } else if(onvif_dev->last_error == ONVIF_CONNECTION_ERROR) {
        printf("An conncetion error was encountered\n");
        OnvifDevice__destroy(onvif_dev);
    } else if(onvif_dev->last_error == ONVIF_SOAP_ERROR) {
        printf("An soap error was encountered \n");
        OnvifDevice__destroy(onvif_dev);
    }

exit:
    free(event);
}

void add_device_add_cb(AppDialogEvent * event){
    OnvifApp * app = (OnvifApp *) event->user_data;
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

void _overscale_cb(AppSettings * settings, int allow_overscale, void * user_data){
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

    char str[7];
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
    app->queue = EventQueue__create(eventqueue_dispatch_cb,app);
    app->details = OnvifDetails__create(app->queue);
    app->settings = AppSettings__create(app->queue);
    app->taskmgr = TaskMgr__create();

    AppSettings__set_overscale_callback(app->settings,_overscale_cb,app);
    app->current_page = 0;
    app->player = RtspPlayer__create();
    RtspPlayer__allow_overscale(app->player,AppSettings__get_allow_overscale(app->settings));

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
        RtspPlayer__destroy(self->player);
        CObject__destroy((CObject*)self->queue);
        CObject__destroy((CObject *)self->device_list);
        gtk_widget_destroy(self->window);
        free(self);
    }
}

void profile_callback(Device * device, void * user_data){
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
    for (b=0;b<self->device_list->count;b++){
        Device * dev = (Device *) self->device_list->data[b];
        OnvifDevice * odev = Device__get_device(dev);
        printf("DEBUG List Record :[%i] %s:%s\n",b,odev->ip,odev->port);
    }

    GtkWidget * row = Device__create_row(device, onvif_dev->device_soap->endpoint, name, hardware, location);
    
    gtk_list_box_insert (GTK_LIST_BOX (self->listbox), row, -1);
    gtk_widget_show_all (row);

    onvif_display_device_row(self,device, 0);
}