#include <string.h>
#include "gst2/player.h"
#include "discoverer.h"
#include "onvif_discovery.h"
#include "onvif_list.h"
#include "onvif_device.h"
#include "queue/event_queue.h"
#include "gui/gui_update_calls.h"
#include "gui/credentials_input.h"
#include "gst2/onvifinitstaticplugins.h"

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/pbutils/gstpluginsbaseversion.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

EventQueue* EVENT_QUEUE;

/* This function is called when the STOP button is clicked */
static void stop_cb (GtkButton *button, OnvifPlayer *data) {
  OnvifPlayer__stop(data);
}

/* This function is called when the STOP button is clicked */
static void quit_cb (GtkButton *button, OnvifPlayer *data) {
  stop_cb (button, data);
  gtk_main_quit();
}

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, OnvifPlayer *data) {
  stop_cb (NULL, data);
  gtk_main_quit ();
}

/*
 * listitem_selected
 *
 * Event handler called when an item is selected.
 */
void row_selected_cb (GtkWidget *widget,   GtkListBoxRow* row,
  OnvifPlayer* player)
{
  select_onvif_device_row(player,row,EVENT_QUEUE);    
}

static GtkWidget *
create_row (ProbMatch * m, OnvifPlayer *player)
{
  GtkWidget *row;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *image;

  int i;
  printf("--- Prob Match ---\n");
  printf("\tProbe %s\n",m->prob_uuid);
  printf("\tTypes %s\n",m->types);
  printf("\tVersion : %i\n", m->version);
  printf("\tscope count : %i\n", m->scope_count);
  for (i = 0 ; i < m->scope_count ; ++i) {
    printf("\t\tScope : %s\n",m->scopes[i]);
  }
  printf("\tAddr count : %i\n", m->addrs_count);
  for (i = 0 ; i < m->addrs_count ; ++i) {
    printf("\t\tAddr : %s\n",m->addrs[i]);
  }
  printf("------------------\n");

  //TODO Support IPv6 alternate IP
  OnvifDevice * onvif_dev = OnvifDevice__create(g_strdup(m->addrs[0]));
  Device * device = Device_create(onvif_dev);
  DeviceList__insert_element(player->device_list,device,player->device_list->device_count);
  int b;
  for (b=0;b<player->device_list->device_count;b++){
    printf("DEBUG List Record :[%i] %s:%s\n",b,player->device_list->devices[b]->onvif_device->ip,player->device_list->devices[b]->onvif_device->port);
  }

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

  char* m_name = onvif_extract_scope("name",m);
  char* markup_name = malloc(strlen(m_name)+1+7);
  strcpy(markup_name, "<b>");
  strcat(markup_name, m_name);
  strcat(markup_name, "</b>");
  label = gtk_label_new (NULL);
  gtk_label_set_markup(GTK_LABEL(label), markup_name);
  g_object_set (label, "margin-end", 5, NULL);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 2, 1);

  label = gtk_label_new (onvif_dev->ip);
  g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);

  label = gtk_label_new (onvif_extract_scope("hardware",m));
  g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);

  label = gtk_label_new (onvif_extract_scope("location",m));
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

  //Dispatch Thread event to update GtkListRow
  display_onvif_device_row(device,EVENT_QUEUE);

  return row;
}

struct DiscoveryInput {
  OnvifPlayer * player;
  GtkWidget * widget;
};

static gboolean * 
finished_discovery (void * e)
{
  DiscoveryEvent * event = (DiscoveryEvent *) e;
  struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;
  
  gtk_widget_set_sensitive(disco_in->widget,TRUE);
  free(disco_in);
  return FALSE;
}
static gboolean * 
found_server (void * e)
{
  GtkWidget *row;
  DiscoveryEvent * event = (DiscoveryEvent *) e;

  DiscoveredServer * server = event->server;
  struct DiscoveryInput * disco_in = (struct DiscoveryInput * ) event->data;

  ProbMatch * m;
  int i;
  
  printf("Found server ---------------- %i\n",server->matches->match_count);
  for (i = 0 ; i < server->matches->match_count ; ++i) {
    m = server->matches->matches[i];

    // Create GtkListBoxRow and add it
    row = create_row (m,disco_in->player);
    gtk_list_box_insert (GTK_LIST_BOX (disco_in->player->listbox), row, -1);  
  }
  //Display new content
  gtk_widget_show_all (disco_in->player->listbox);

  return FALSE;
}

void
onvif_scan (GtkWidget *widget,
             OnvifPlayer *player)
{

  gtk_widget_set_sensitive(widget,FALSE);

  //Clearing the list
  gtk_container_foreach (GTK_CONTAINER (player->listbox), (GtkCallback)gtk_widget_destroy, NULL);
  
  //Clearing old list data
  DeviceList__clear(player->device_list);

  struct DiscoveryInput * disco_in = malloc(sizeof(struct DiscoveryInput));
  disco_in->player = player;
  disco_in->widget = widget;
  //Start UDP Scan
  struct UdpDiscoverer discoverer = UdpDiscoverer__create(found_server,finished_discovery);
  UdpDiscoverer__start(&discoverer, disco_in);

}

GtkWidget * add_label_entry(GtkWidget * grid, int row, char* lbl){
  GtkWidget * widget;

  widget = gtk_label_new (lbl);
  gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);
  widget = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(widget),"SomeName");
  gtk_editable_set_editable  ((GtkEditable*)widget, FALSE);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, row, 1, 1);

  return widget;
}

GtkWidget * create_info_ui(OnvifPlayer *player){
    GtkWidget *grid;

    grid = gtk_grid_new ();

    add_label_entry(grid,0,"Hostname : ");

    add_label_entry(grid,1,"Location : ");

    add_label_entry(grid,2,"Manufacturer : ");

    add_label_entry(grid,3,"Model : ");

    add_label_entry(grid,4,"Hardware : ");

    add_label_entry(grid,5,"Firmware : ");

    add_label_entry(grid,6,"IP Address : ");

    add_label_entry(grid,7,"MAC Address : ");

    add_label_entry(grid,8,"ONVIF Version : ");

    add_label_entry(grid,9,"URI : ");

    return grid;
}


void update_details_page(gint index, OnvifPlayer * player){
  printf("update_details_page %i\n",index);
  if(index == 0){ //Information Page
    if(!player->device){
      printf("No device selected...\n");
      //TODO Clear
    } else {
      printf("dev : %s\n",player->device->onvif_device->ip);
    }
    // OnvifDeviceInformation * dev_info = OnvifDevice__device_getDeviceInformation(dev);
    // printf("manufacturer : %s\n",dev_info->manufacturer);
    // printf("model %s\n",dev_info->model);
    // printf("firmwareVersion %s\n",dev_info->firmwareVersion);
    // printf("serialNumber %s\n",dev_info->serialNumber);
    // printf("hardwareId %s\n",dev_info->hardwareId);
    // char * manufacturer;
    // char * model;
    // char * firmwareVersion;
    // char * serialNumber;
    // char * hardwareId;
  }
}

static void switch_detail_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifPlayer * player) {
  gboolean ret = gtk_widget_is_visible(page);
  if(!ret){//Only trigger gui update if page is visisble
    return;
  }
  update_details_page(page_num,player);
}


GtkWidget * create_details_ui (OnvifPlayer *player){
  GtkWidget * widget;
  GtkWidget *label;
  char * TITLE_STR = "Information";

  player->details_notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (player->details_notebook), GTK_POS_LEFT);

  widget = create_info_ui(player);

  label = gtk_label_new (TITLE_STR);
  gtk_notebook_append_page (GTK_NOTEBOOK (player->details_notebook), widget, label);

  return player->details_notebook;
}


GtkWidget * create_nvt_ui (OnvifPlayer *player){
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

void switch_ui_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifPlayer * player) {
    printf("switch_ui_page %i\n",page_num);
    gboolean ret = gtk_widget_is_visible(page);
    if(!ret){//Only trigger gui update if page is visisble
      return;
    }
    if(page_num == 0){ //NVT

    } else if(page_num == 1){
      gint selected_index = gtk_notebook_get_current_page(GTK_NOTEBOOK(player->details_notebook));
      update_details_page(selected_index,player);
    } //Else ??
}

void create_ui (OnvifPlayer* player) {
  GtkWidget *main_window;  /* The uppermost window, containing all other windows */
  GtkWidget *grid;
  GtkWidget *widget;
  GtkWidget *notebook;
  GtkWidget *label;
  
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), player);

  gtk_window_set_title (GTK_WINDOW (main_window), "Onvif Device Manager");

  GtkWidget * overlay =gtk_overlay_new();
  gtk_container_add (GTK_CONTAINER (main_window), overlay);

  /* Here we construct the container that is going pack our buttons */
  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 10);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay),grid);

  player->dialog = CredentialsDialog__create(dialog_login_cb, dialog_cancel_cb);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay),player->dialog->root);

  widget = gtk_button_new ();
  label = gtk_label_new("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\">Scan</span>...");
  gtk_container_add (GTK_CONTAINER (widget), label);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);
  gtk_widget_set_size_request(widget,-1,80);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);


  /* --- Create a list item from the data element --- */
  player->listbox = gtk_list_box_new ();
  gtk_widget_set_size_request(player->listbox,100,-1);
  gtk_widget_set_vexpand (player->listbox, TRUE);
  gtk_widget_set_hexpand (player->listbox, FALSE);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (player->listbox), GTK_SELECTION_SINGLE);
  g_signal_connect (player->listbox, "row-selected", G_CALLBACK (row_selected_cb), player);
  gtk_grid_attach (GTK_GRID (grid), player->listbox, 0, 1, 1, 1);
  g_signal_connect (widget, "clicked", G_CALLBACK (onvif_scan), player);

  widget = gtk_button_new_with_label ("Quit");
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);
  g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK (quit_cb), player);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 1, 1);

  //Padding (TODO Implement draggable resizer)
  widget = gtk_label_new("");
  gtk_widget_set_vexpand (widget, TRUE);
  gtk_widget_set_hexpand (widget, FALSE);
  gtk_widget_set_size_request(widget,20,-1);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 3);

  /* Create a new notebook, place the position of the tabs */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_widget_set_vexpand (notebook, TRUE);
  gtk_widget_set_hexpand (notebook, TRUE);
  gtk_grid_attach (GTK_GRID (grid), notebook, 2, 0, 1, 3);


  widget = create_nvt_ui(player);

  char * TITLE_STR = "NVT";
  label = gtk_label_new (TITLE_STR);

  //Hidden spinner used to display stream start loading
  player->loading_handle = gtk_spinner_new ();
  gtk_spinner_start (GTK_SPINNER (player->loading_handle));

  //Only show label and keep loading hidden
  GtkWidget * hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  6);
  gtk_box_pack_start (GTK_BOX(hbox),label,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hbox),player->loading_handle,FALSE,FALSE,0);
  gtk_widget_show(label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, hbox);

  widget = create_details_ui(player);
  label = gtk_label_new ("Details");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, label);

  gtk_window_set_default_size(GTK_WINDOW(main_window),640,480);
  gtk_widget_show_all (main_window);
  g_signal_connect (G_OBJECT (notebook), "switch-page", G_CALLBACK (switch_ui_page), player);
  g_signal_connect (G_OBJECT (player->details_notebook), "switch-page", G_CALLBACK (switch_detail_page), player);

}


int main(int argc, char *argv[]) {
  OnvifPlayer * data;
  EVENT_QUEUE = EventQueue__create();
  //Defaults 4 paralell event threads.
  //TODO support configuration to modify this
  EventQueue__start(EVENT_QUEUE);
  EventQueue__start(EVENT_QUEUE);
  EventQueue__start(EVENT_QUEUE);
  EventQueue__start(EVENT_QUEUE);

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  onvif_init_static_plugins();

  printf("Using Gstreamer Version : %i.%i.%i.%i\n",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);
  
  /* Initialize our data structure */
  data = OnvifPlayer__create();
  
  /* Create the GUI */
  create_ui (data);

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  /* Free resources */
  gst_object_unref (data->pipeline);
  return 0;
}
