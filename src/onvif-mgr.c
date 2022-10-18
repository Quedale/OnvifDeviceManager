#include <string.h>
#include "gst2/player.h"
#include "udp/soap_parser.h"
#include "udp/discoverer.h"
#include "soap/client.h"
#include "ui/onvif_list.h"
#include "ui/onvif_device.h"

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

/* This function is called when the PLAY button is clicked */
static void play_cb (GtkButton *button, OnvifPlayer *data) {
  gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

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
    stop_cb (NULL, player);
    //Unselected. Stopping stream TODO this isn't enough
    if(row == NULL){
      return;
    }
    int pos;
    pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
    
    OnvifDevice dev = player->onvifDeviceList->devices[pos];
    
    /* Set the URI to play */
    char * uri = OnvifDevice__media_getStreamUri(&dev);

    OnvifPlayer__set_playback_url(player,uri);
    
    OnvifPlayer__play(player);
    
}

static GtkWidget *
create_row (struct ProbMatch * m, OnvifPlayer *player)
{
  // GtkWidget *row, *handle, *box, *label, *image;
  GtkWidget *row;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *handle;
  GtkWidget *image;

  int i;
  printf("--- Prob -------\n");
  printf("\tProbe %s\n",m->prob_uuid);
  printf("\tAddr %s\n",m->addr);
  printf("\tTypes %s\n",m->types);
  printf("\tVersion : %s\n", m->version);
  printf("\tcount : %i\n", m->scope_count);
  for (i = 0 ; i < m->scope_count ; ++i) {
    printf("\t\tScope : %s\n",m->scopes[i]);
  }
  
  OnvifDevice dev = OnvifDevice__create(m->addr);
  OnvifDeviceInformation * devinfo = OnvifDevice__device_getDeviceInformation(&dev);
  // printf("--- Device -------\n");
  // printf("\tfirmware : %s\n",devinfo->firmwareVersion);
  // printf("\thardware : %s\n",devinfo->hardwareId);
  // printf("\tmanufacturer : %s\n",devinfo->manufacturer);
  // printf("\tmodel : %s\n",devinfo->model);
  // printf("\tserial# : %s\n",devinfo->serialNumber);

  OnvifDeviceList__insert_element(player->onvifDeviceList,dev,player->onvifDeviceList->device_count);
  int b;
  for (b=0;b<player->onvifDeviceList->device_count;b++){
    printf("DEBUG List Record :[%i] %s\n",b,player->onvifDeviceList->devices[b].hostname);
  }

  row = gtk_list_box_row_new ();

  grid = gtk_grid_new ();
  g_object_set (grid, "margin", 5, NULL);
  gtk_container_add (GTK_CONTAINER (row), grid);

  handle = gtk_event_box_new ();
  image = gtk_image_new_from_icon_name ("open-menu-symbolic", 1);
  gtk_container_add (GTK_CONTAINER (handle), image);
  g_object_set (handle, "margin-end", 10, NULL);
  // gtk_container_add (GTK_CONTAINER (grid), handle);
  gtk_grid_attach (GTK_GRID (grid), handle, 0, 0, 1, 3);

  label = gtk_label_new (dev.hostname);
  g_object_set (label, "margin-end", 5, NULL);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);

  label = gtk_label_new (devinfo->manufacturer);
  g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);

  label = gtk_label_new (devinfo->model);
  g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);


  return row;
}

static gboolean * 
finished_discovery (void * e)
{
  DiscoveryEvent * event = (DiscoveryEvent *) e;
  GtkWidget * widget = event->widget;
  
  gtk_widget_set_sensitive(widget,TRUE);

  return FALSE;
}
static gboolean * 
found_server (void * e)
{
  GtkWidget *row;
  DiscoveryEvent * event = (DiscoveryEvent *) e;
  DiscoveredServer * server = event->server;
  OnvifPlayer * player = (OnvifPlayer *) event->player;
  
  struct ProbMatch m;
  int i;
  
  printf("Found server ----------------\n");
  for (i = 0 ; i < server->match_count ; ++i) {
    m = server->matches[i];

    // Create GtkListBoxRow and add it
    printf("Prob Match #%i ----------------\n",i);
    row = create_row (&m,player);
    gtk_list_box_insert (GTK_LIST_BOX (player->listbox), row, -1);  
  }
  //Display new content
  gtk_widget_show_all (player->listbox);

  return FALSE;
}
void
onvif_scan (GtkWidget *widget,
             OnvifPlayer *player)
{
  gchar *text;
  GtkWidget *row;

  gtk_widget_set_sensitive(widget,FALSE);

  //Clearing the list
  gtk_container_foreach (GTK_CONTAINER (player->listbox), (GtkCallback)gtk_widget_destroy, NULL);
  
  //Clearing old list data
  OnvifDeviceList__clear(player->onvifDeviceList);

  //Start UDP Scan
  struct UdpDiscoverer discoverer = UdpDiscoverer__create(found_server,finished_discovery);
  UdpDiscoverer__start(&discoverer, widget, player);

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
    GtkWidget * widget;

    grid = gtk_grid_new ();

    widget = add_label_entry(grid,0,"Hostname : ");

    widget = add_label_entry(grid,1,"Location : ");

    widget = add_label_entry(grid,2,"Manufacturer : ");

    widget = add_label_entry(grid,3,"Model : ");

    widget = add_label_entry(grid,4,"Hardware : ");

    widget = add_label_entry(grid,5,"Firmware : ");

    widget = add_label_entry(grid,6,"IP Address : ");

    widget = add_label_entry(grid,7,"MAC Address : ");

    widget = add_label_entry(grid,8,"ONVIF Version : ");

    widget = add_label_entry(grid,9,"URI : ");

    return grid;
}

GtkWidget * create_details_ui (OnvifPlayer *player){
  GtkWidget * notebook;
  GtkWidget * widget;
  GtkWidget *label;
  char * TITLE_STR = "Information";

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);

  widget = create_info_ui(player);

  label = gtk_label_new (TITLE_STR);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, label);

  return notebook;
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

  return grid;
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
  gtk_container_set_border_width (GTK_CONTAINER (main_window), 10);

  /* Here we construct the container that is going pack our buttons */
  grid = gtk_grid_new ();

  gtk_container_add (GTK_CONTAINER (main_window), grid);
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
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, label);

  widget = create_details_ui(player);
  label = gtk_label_new ("Details");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, label);

  gtk_window_set_default_size(GTK_WINDOW(main_window),640,480);
  gtk_widget_show_all (main_window);
}



int main(int argc, char *argv[]) {
  OnvifPlayer * data;

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Initialize our data structure */
  data = OnvifPlayer__create();
  
  /* Create the GUI */
  create_ui (data);

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  /* Free resources */
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
  gst_object_unref (data->pipeline);
  return 0;
}
