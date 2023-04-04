#include "onvif_app.h"
#include "../gui/credentials_input.h"
#include "../device_list.h"
#include "../queue/event_queue.h"
#include "../gst2/player.h"
#include "discoverer.h"
#include "onvif_info_details.h"
#include <netdb.h>
#include <arpa/inet.h>

extern char _binary_prohibited_icon_png_size[];
extern char _binary_prohibited_icon_png_start[];
extern char _binary_prohibited_icon_png_end[];

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

extern char _binary_warning_png_size[];
extern char _binary_warning_png_start[];
extern char _binary_warning_png_end[];

extern char _binary_microphone_png_size[];
extern char _binary_microphone_png_start[];
extern char _binary_microphone_png_end[];

typedef struct _OnvifApp {
    Device* device; /* Currently selected device */
    DeviceList *device_list;

    GtkWidget *listbox;
    GtkWidget *main_notebook;
    GtkWidget *details_notebook;
    GtkWidget *player_loading_handle;

    OnvifInfoDetails * onvif_info_details;
    CredentialsDialog * dialog;

    int current_page;

    EventQueue * queue;
    RtspPlayer * player;
} OnvifApp;

struct PlayInput {
    OnvifApp * app;
    Device * device;
    GtkListBoxRow * row;
};

struct DeviceInput {
    Device * device;
    OnvifApp * app;
};

struct DiscoveryInput {
    OnvifApp * app;
    GtkWidget * widget;
};


struct DeviceInput * DeviceInput_copy(struct DeviceInput * input){
    struct DeviceInput * newinput = malloc(sizeof(struct DeviceInput));
    newinput->device = input->device;
    newinput->app = input->app;
    return newinput;
}

struct PlayInput * PlayInput_copy(struct PlayInput * input){
    struct PlayInput * newinput = malloc(sizeof(struct PlayInput));
    newinput->app = input->app;
    newinput->row = input->row;
    newinput->app = input->app;
    newinput->device = input->device;
    return newinput;
}

/*
 *
 *  Gtk GUI Signal Callbacks
 *
 */

/* This function is called when the STOP button is clicked */
static void quit_cb (GtkButton *button, RtspPlayer *data) {
    RtspPlayer__stop(data);
    gtk_main_quit();
}

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, RtspPlayer *data) {
    RtspPlayer__stop(data);
    gtk_main_quit ();
}

static gboolean * finished_discovery (void * e) {
    DiscoveryEvent * event = (DiscoveryEvent *) e;
    struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;
    
    gtk_widget_set_sensitive(disco_in->widget,TRUE);
    free(disco_in);
    return FALSE;
}

void add_device(OnvifApp * self, char * uri, char* name, char * hardware, char * location);

static gboolean * found_server (void * e) {
    DiscoveryEvent * event = (DiscoveryEvent *) e;

    DiscoveredServer * server = event->server;
    struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;

    ProbMatch * m;
    int i;
  
    printf("Found server ---------------- %i\n",server->matches->match_count);
    for (i = 0 ; i < server->matches->match_count ; ++i) {
        m = server->matches->matches[i];
        add_device(disco_in->app, g_strdup(m->addrs[0]), onvif_extract_scope("name",m), onvif_extract_scope("hardware",m), onvif_extract_scope("location",m));
    }

    return FALSE;
}

void onvif_scan (GtkWidget *widget, OnvifApp * app) {

    gtk_widget_set_sensitive(widget,FALSE);

    //Clearing the list
    gtk_container_foreach (GTK_CONTAINER (app->listbox), (GtkCallback)gtk_widget_destroy, NULL);
    DeviceList__clear(app->device_list);

    struct DiscoveryInput * disco_in = malloc(sizeof(struct DiscoveryInput));
    disco_in->app = app;
    disco_in->widget = widget;
    //Start UDP Scan
    struct UdpDiscoverer discoverer = UdpDiscoverer__create(found_server,finished_discovery);
    UdpDiscoverer__start(&discoverer, disco_in);

}

gboolean toggle_mic_cb (GtkWidget *widget, gpointer * p, gpointer * p2){
    RtspPlayer * player = (RtspPlayer *) p2;
    if(RtspPlayer__is_mic_mute(player)){
        RtspPlayer__mic_mute(player,FALSE);
    } else {
        RtspPlayer__mic_mute(player,TRUE);
    }
    return FALSE;
}

void error_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    gtk_widget_hide(app->player_loading_handle);
}

void stopped_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    gtk_widget_hide(app->player_loading_handle);
}

void start_onvif_stream(RtspPlayer * player, void * user_data){
    OnvifApp * app = (OnvifApp *) user_data;
    gtk_widget_show(app->player_loading_handle);
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

void _display_onvif_thumbnail(void * user_data, int profile_index){
    GtkWidget *image;
    GdkPixbufLoader *loader = NULL;
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled_pixbuf = NULL;
    double size;
    char * imgdata;
    int freeimgdata = 0;

    struct DeviceInput * input = (struct DeviceInput *) user_data;

    if(!Device__addref(input->device)){
        goto exit;
    }

    if(input->device->onvif_device->authorized){
        //TODO handle profiles
        struct chunk * imgchunk = OnvifDevice__media_getSnapshot(input->device->onvif_device,profile_index);
        if(!imgchunk){
            //TODO Set error image
            printf("Error retrieve snapshot.");
            goto exit;
        }
        imgdata = imgchunk->buffer;
        size = imgchunk->size;
        freeimgdata = 1;
        free(imgchunk);
    } else {
        imgdata = _binary_locked_icon_png_start;
        size = _binary_locked_icon_png_end - _binary_locked_icon_png_start;
    }

    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!Device__is_valid(input->device)){
        goto exit;
    }

    //Attempt to get downloaded pixbuf or locked icon
    loader = gdk_pixbuf_loader_new ();
    GError *error = NULL;
    if(gdk_pixbuf_loader_write (loader, (unsigned char *)imgdata, size,&error)){
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    } else {
        if(error->message){
            printf("Error writing png to GtkPixbufLoader : %s\n",error->message);
        } else {
            printf("Error writing png to GtkPixbufLoader : [null]\n");
        }
    }
  
    //Show warning icon in case of failure
    if(!pixbuf){
        if(gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_warning_png_start, _binary_warning_png_end - _binary_warning_png_start,&error)){
            pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        } else {
            if(error->message){
                printf("Error writing warning png to GtkPixbufLoader : %s\n",error->message);
            } else {
                printf("Error writing warning png to GtkPixbufLoader : [null]\n");
            }
        }
    }

    //Check is device is still valid. (User performed scan before spinner showed)
    if(!Device__is_valid(input->device)){
        goto exit;
    }

    //Remove previous image
    gtk_container_foreach (GTK_CONTAINER (input->device->image_handle), (void*) gtk_widget_destroy, NULL);

    //Display pixbuf
    if(pixbuf){
        double ph = gdk_pixbuf_get_height (pixbuf);
        double pw = gdk_pixbuf_get_width (pixbuf);
        double newpw = 40 / ph * pw;
        scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,40,GDK_INTERP_NEAREST);
        image = gtk_image_new_from_pixbuf (scaled_pixbuf);

        //Check is device is still valid. (User performed scan before scale finished)
        if(!Device__is_valid(input->device)){
            goto exit;
        }

        gtk_container_add (GTK_CONTAINER (input->device->image_handle), image);
        gtk_widget_show (image);
    } else {
        printf("Failed all thumbnail creation.\n");
    }

exit:
    if(loader){
        gdk_pixbuf_loader_close(loader,NULL);
        g_object_unref(loader);
    }
    if(scaled_pixbuf){
        g_object_unref(scaled_pixbuf);
    }
    if(freeimgdata){
        free(imgdata);
    }
    Device__unref(input->device);
}

//WIP Profile selection
void _display_onvif_profiles(void * user_data){
    struct DeviceInput * input = (struct DeviceInput *) user_data;

    if(!input->device->onvif_device->authorized){
        return;
    }

    GtkListStore *liststore = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(input->device->profile_dropdown)));
    OnvifDevice_get_profiles(input->device->onvif_device);
    for (int i = 0; i < input->device->onvif_device->sizeSrofiles; i++){
        printf("Profile name: %s\n", input->device->onvif_device->profiles[i].name);
        printf("Profile token: %s\n", input->device->onvif_device->profiles[i].token);

        // if(i == 0){
        //   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(input->device->profile_dropdown)->entry), input->device->onvif_device->profiles[i].name);
        // }

        // GtkListStore *liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
        gtk_list_store_insert_with_values(liststore, NULL, -1,
                                        // 0, "red",
                                        1, input->device->onvif_device->profiles[i].name,
                                        // 2, input->device->onvif_device->profiles[i].token,
                                        -1);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(input->device->profile_dropdown),0);
    gtk_widget_set_sensitive (input->device->profile_dropdown, TRUE);
    // gtk_combo_set_popdown_strings (GTK_COMBO(input->device->profile_dropdown), cbitems);

}

void _display_onvif_device(void * user_data){
    struct DeviceInput * input = (struct DeviceInput *) user_data;
    /* Start by authenticating the device then start retrieve thumbnail */
    OnvifDevice_authenticate(input->device->onvif_device);

    /* Display Profile dropdown */
    // _display_onvif_profiles(input);

    /* Display row thumbnail. Default to profile index 0 */
    _display_onvif_thumbnail(input, 0);

    free(input);
}

void _display_nslookup_hostname(void * user_data){
    struct DeviceInput * input = (struct DeviceInput *) user_data;
    char * hostname;

    if(!Device__addref(input->device)){
        goto exit;
    }

    printf("NSLookup ... %s\n",input->device->onvif_device->ip);
    //Lookup hostname
    struct in_addr in_a;
    inet_pton(AF_INET, input->device->onvif_device->ip, &in_a);
    struct hostent* host;
    host = gethostbyaddr( (const void*)&in_a, 
                        sizeof(struct in_addr), 
                        AF_INET );
    if(host){
        printf("Found hostname : %s\n",host->h_name);
        hostname = host->h_name;
    } else {
        printf("Failed to get hostname ...\n");
        hostname = NULL;
    }

    printf("Retrieved hostname : %s\n",hostname);

exit:
    Device__unref(input->device);
    free(input);
}

gboolean * gdk_thread_dispatch (void * user_data){
    struct PlayInput * input = (struct PlayInput *) user_data;
    CredentialsDialog__show(input->app->dialog,input);
    return FALSE;
}

void onvif_display_device_row(OnvifApp * self, Device * device){
    struct DeviceInput * input = malloc(sizeof(struct DeviceInput));
    input->device = device;
    input->app = self;

    /* nslookup doesn't require onvif authentication. Dispatch event now. */
    EventQueue__insert(self->queue,_display_nslookup_hostname,DeviceInput_copy(input));

    EventQueue__insert(self->queue,_display_onvif_device,input);
}

void _play_onvif_stream(void * user_data){
    struct PlayInput * input = (struct PlayInput *) user_data;

    //Check if device is still valid. (User performed scan before thread started)
    if(!Device__addref(input->device)){
        goto exit;
    }

    if(!input->device->selected){
        return;
    }

        /* Set the URI to play */
        //TODO handle profiles
    char * uri = OnvifDevice__media_getStreamUri(input->device->onvif_device,0);

    RtspPlayer__set_playback_url(input->app->player,uri);

    //User performed scan before StreamURI was retrieved
    if(!Device__is_valid(input->device) || !input->device->selected){
        goto exit;
    }

    RtspPlayer__play(input->app->player);

exit:
    Device__unref(input->device);
    free(input);
}

void update_details(OnvifApp * self){
    if(!self->current_page){//NVT is displayed
        return;
    }
    OnvifInfoDetails_update_details(self->onvif_info_details,self->device,self->queue);
}

void OnvifApp__select_device(OnvifApp * app,  GtkListBoxRow * row){
    struct PlayInput * input = malloc (sizeof(struct PlayInput));
    input->app = app;
    input->row = row;

    OnvifInfoDetails_clear_details(input->app->onvif_info_details);
    EventQueue__insert(app->queue,_stop_onvif_stream,app);
    if(input->app->device){
        input->app->device->selected = 0;
    }

    if(input->row == NULL){
        input->app->device = NULL;
        free(input);
        return;
    }

    //Set newly selected device
    int pos;
    pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (input->row));
    input->app->device = input->app->device_list->devices[pos];
    input->app->device->selected = 1;
    input->device = input->app->device;

    //Prompt for authentication
    if(!input->device->onvif_device->authorized){
        gdk_threads_add_idle((void *)gdk_thread_dispatch,input);
        return;
    }

    EventQueue__insert(app->queue,_play_onvif_stream,input);
    update_details(input->app);
}

void row_selected_cb (GtkWidget *widget,   GtkListBoxRow* row,
  OnvifApp* app)
{
    OnvifApp__select_device(app,row);    
}

static void switch_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifApp * app) {
    OnvifInfoDetails_clear_details(app->onvif_info_details);
    app->current_page = page_num;
    update_details(app);
}

/*
 *
 *  UI Creation
 *
 */

GtkWidget * create_details_ui (OnvifApp * app){
    GtkWidget * widget;
    GtkWidget *label;
    char * TITLE_STR = "Information";

    app->details_notebook = gtk_notebook_new ();

    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (app->details_notebook), GTK_POS_LEFT);

    widget = OnvifInfoDetails__create_ui(app->onvif_info_details);

    label = gtk_label_new (TITLE_STR);
    gtk_notebook_append_page (GTK_NOTEBOOK (app->details_notebook), widget, label);

    return app->details_notebook;
}

GtkWidget * create_controls_overlay(RtspPlayer *player){

    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
    gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_microphone_png_start, _binary_microphone_png_end - _binary_microphone_png_start, NULL);

    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    double ph = gdk_pixbuf_get_height (pixbuf);
    double pw = gdk_pixbuf_get_width (pixbuf);
    double newpw = 30 / ph * pw;
    pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,30,GDK_INTERP_NEAREST);
    GtkWidget * image = gtk_image_new_from_pixbuf (pixbuf); 

    GtkWidget * widget = gtk_button_new ();
    g_signal_connect (widget, "button-press-event", G_CALLBACK (toggle_mic_cb), player);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (toggle_mic_cb), player);

    gtk_button_set_image (GTK_BUTTON (widget), image);

    GtkWidget * fixed = gtk_fixed_new();
    gtk_fixed_put(GTK_FIXED(fixed),widget,10,10); 
    return fixed;
}

GtkWidget * create_nvt_ui (RtspPlayer * player){
    GtkWidget *grid;
    GtkWidget *widget;

    grid = gtk_grid_new ();
    widget = OnvifDevice__createCanvas(player);
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_widget_set_hexpand (widget, TRUE);

    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black;}",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(grid);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssProvider);  

    GtkWidget * overlay =gtk_overlay_new();
    gtk_container_add (GTK_CONTAINER (overlay), grid);

    widget = create_controls_overlay(player);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),widget);
    return overlay;
}


void create_ui (OnvifApp * app) {
    GtkWidget *main_window;  /* The uppermost window, containing all other windows */
    GtkWidget *grid;
    GtkWidget *widget;
    GtkWidget *label;
    GtkWidget * hbox;

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), app->player);

    gtk_window_set_title (GTK_WINDOW (main_window), "Onvif Device Manager");

    GtkWidget * overlay =gtk_overlay_new();
    gtk_container_add (GTK_CONTAINER (main_window), overlay);

    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new ();
    gtk_container_set_border_width (GTK_CONTAINER (grid), 10);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),grid);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),app->dialog->root);

    widget = gtk_button_new ();
    label = gtk_label_new("");
    gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\">Scan</span>...");
    gtk_container_add (GTK_CONTAINER (widget), label);
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_widget_set_hexpand (widget, FALSE);
    gtk_widget_set_size_request(widget,-1,80);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);
    g_signal_connect (widget, "clicked", G_CALLBACK (onvif_scan), app);

    /* --- Create a list item from the data element --- */
    app->listbox = gtk_list_box_new ();
    gtk_widget_set_size_request(app->listbox,100,-1);
    gtk_widget_set_vexpand (app->listbox, TRUE);
    gtk_widget_set_hexpand (app->listbox, FALSE);
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (app->listbox), GTK_SELECTION_SINGLE);
    gtk_grid_attach (GTK_GRID (grid), app->listbox, 0, 1, 1, 1);
    g_signal_connect (app->listbox, "row-selected", G_CALLBACK (row_selected_cb), app);

    widget = gtk_button_new_with_label ("Quit");
    gtk_widget_set_vexpand (widget, FALSE);
    gtk_widget_set_hexpand (widget, FALSE);
    g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK (quit_cb), app);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);

    //Padding (TODO Implement draggable resizer)
    widget = gtk_label_new("");
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_widget_set_hexpand (widget, FALSE);
    gtk_widget_set_size_request(widget,20,-1);
    gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 3);

    /* Create a new notebook, place the position of the tabs */
    app->main_notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (app->main_notebook), GTK_POS_TOP);
    gtk_widget_set_vexpand (app->main_notebook, TRUE);
    gtk_widget_set_hexpand (app->main_notebook, TRUE);
    gtk_grid_attach (GTK_GRID (grid), app->main_notebook, 2, 0, 1, 3);

    char * TITLE_STR = "NVT";
    label = gtk_label_new (TITLE_STR);

    //Hidden spinner used to display stream start loading
    app->player_loading_handle = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (app->player_loading_handle));

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),app->player_loading_handle,FALSE,FALSE,0);
    gtk_widget_show(label);

    widget = create_nvt_ui(app->player);
    gtk_notebook_append_page (GTK_NOTEBOOK (app->main_notebook), widget, hbox);

    label = gtk_label_new ("Details");

    //Hidden spinner used to display stream start loading
    widget = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (widget));
    OnvifInfoDetails_set_details_loading_handle(app->onvif_info_details,widget);

    //Only show label and keep loading hidden
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
    gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),widget,FALSE,FALSE,0);
    gtk_widget_show(label);

    widget = create_details_ui(app);

    gtk_notebook_append_page (GTK_NOTEBOOK (app->main_notebook), widget, hbox);

    gtk_window_set_default_size(GTK_WINDOW(main_window),640,480);
    gtk_widget_show_all (main_window);
    g_signal_connect (G_OBJECT (app->main_notebook), "switch-page", G_CALLBACK (switch_page), app);

}

static GtkWidget *
create_device_row (OnvifApp * app, Device * device, char * uri, char* name, char * hardware, char * location)
{
    GtkWidget *row;
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *image;
    
    row = gtk_list_box_row_new ();

    grid = gtk_grid_new ();
    g_object_set (grid, "margin", 5, NULL);

    image = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (image));

    GtkWidget * thumbnail_handle = gtk_event_box_new ();
    device->image_handle = thumbnail_handle;
    gtk_container_add (GTK_CONTAINER (thumbnail_handle), image);
    g_object_set (thumbnail_handle, "margin-end", 10, NULL);
    gtk_grid_attach (GTK_GRID (grid), thumbnail_handle, 0, 1, 1, 3);

    char* markup_name = malloc(strlen(name)+1+7);
    strcpy(markup_name, "<b>");
    strcat(markup_name, name);
    strcat(markup_name, "</b>");
    label = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup_name);
    g_object_set (label, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 2, 1);

    label = gtk_label_new (device->onvif_device->ip);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);

    label = gtk_label_new (hardware);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);

    label = gtk_label_new (location);
    gtk_label_set_ellipsize (GTK_LABEL(label),PANGO_ELLIPSIZE_END);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);

//WIP - ONVIF profile selection
/* 
  GtkListStore *liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  device->profile_dropdown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(liststore));
  // liststore is now owned by combo, so the initial reference can be dropped 
  g_object_unref(liststore);

  GtkCellRenderer  * column = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(device->profile_dropdown), column, TRUE);

  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(device->profile_dropdown), column,
                                  "cell-background", 0,
                                  "text", 1,
                                  NULL);
                                  
  gtk_widget_set_sensitive (device->profile_dropdown, FALSE);
  gtk_grid_attach (GTK_GRID (grid), device->profile_dropdown, 0, 4, 2, 1);
*/

    gtk_container_add (GTK_CONTAINER (row), grid);
  
    //For some reason, spinner has a floating ref
    //This is required to keep ability to remove the spinner later
    g_object_ref(image);

    return row;
}

void _onvif_authentication(void * user_data){
    LoginEvent * event = (LoginEvent *) user_data;
    struct PlayInput * input = (struct PlayInput *) event->user_data;
    //Check device is still valid before adding ref (User performed scan before thread started)
    if(!Device__addref(input->device)){
        goto exit;
    }

    OnvifDevice_set_credentials(input->device->onvif_device,event->user,event->pass);
    OnvifDevice_authenticate(input->device->onvif_device);

    //Check if device is valid and authorized (User performed scan before auth finished)
    if(!Device__is_valid(input->device) || !input->device->onvif_device->authorized || !input->device->selected){
        goto exit;
    }

    //Replace locked image with spinner
    GtkWidget * image = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (image));
    gtk_container_foreach (GTK_CONTAINER (input->device->image_handle), (void*) gtk_widget_destroy, NULL);
    gtk_container_add (GTK_CONTAINER (input->device->image_handle), image);
    gtk_widget_show (image);

    OnvifInfoDetails_clear_details(input->app->onvif_info_details);
    CredentialsDialog__hide(input->app->dialog);
    onvif_display_device_row(input->app, input->device);
    EventQueue__insert(input->app->queue,_play_onvif_stream,PlayInput_copy(input));
    update_details(input->app);

exit:
    Device__unref(input->device);
    free(input);
    free(event);
}

void dialog_cancel_cb(CredentialsDialog * dialog){
    printf("OnvifAuthentication cancelled...\n");
    CredentialsDialog__hide(dialog);
    free(dialog->user_data);
}

void dialog_login_cb(LoginEvent * event){
    printf("OnvifAuthentication attempt...\n");
    struct PlayInput * input = (struct PlayInput *) event->user_data;
    EventQueue__insert(input->app->queue,_onvif_authentication,LoginEvent_copy(event));
}

OnvifApp * OnvifApp__create(){
    OnvifApp *app  =  malloc(sizeof(OnvifApp));
    app->player = RtspPlayer__create();
    app->device_list = DeviceList__create();
    app->device = NULL;
    app->dialog = CredentialsDialog__create(dialog_login_cb, dialog_cancel_cb);
    app->queue = EventQueue__create();
    app->onvif_info_details = OnvifInfoDetails__create();
    app->current_page = 0;

    //Defaults 4 paralell event threads.
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
        OnvifInfoDetails__destroy(self->onvif_info_details);
        CredentialsDialog__destroy(self->dialog);
        RtspPlayer__destroy(self->player);
        EventQueue__destroy(self->queue);
        DeviceList__destroy(self->device_list);
        free(self);
    }
}

void add_device(OnvifApp * self, char * uri, char* name, char * hardware, char * location){
    OnvifDevice * onvif_dev = OnvifDevice__create(uri);
    Device * device = Device_create(onvif_dev);
    DeviceList__insert_element(self->device_list,device,self->device_list->device_count);
    int b;
    for (b=0;b<self->device_list->device_count;b++){
        printf("DEBUG List Record :[%i] %s:%s\n",b,self->device_list->devices[b]->onvif_device->ip,self->device_list->devices[b]->onvif_device->port);
    }

    GtkWidget * row = create_device_row(self, device, uri, name, hardware, location);
    
    gtk_list_box_insert (GTK_LIST_BOX (self->listbox), row, -1);
    gtk_widget_show_all (row);

    onvif_display_device_row(self,device);
}