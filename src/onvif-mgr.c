#include <string.h>
#include "gst2/player.h"
#include "udp/discovery.h"
#include "udp/soap_parser.h"
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
  gst_element_set_state (data->pipeline, GST_STATE_READY);
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
  GtkWidget *row, *handle, *box, *label, *image;

  OnvifDevice dev = OnvifDevice__create(m->addr);

  OnvifDeviceList__insert_element(player->onvifDeviceList,dev,player->onvifDeviceList->device_count);
  int b;
  for (b=0;b<player->onvifDeviceList->device_count;b++){
    printf("DEBUG List Record :[%i] %s\n",b,player->onvifDeviceList->devices[b].hostname);
  }

  row = gtk_list_box_row_new ();

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  gtk_container_add (GTK_CONTAINER (row), box);

  handle = gtk_event_box_new ();
  image = gtk_image_new_from_icon_name ("open-menu-symbolic", 1);
  gtk_container_add (GTK_CONTAINER (handle), image);
  gtk_container_add (GTK_CONTAINER (box), handle);

  label = gtk_label_new (dev.hostname);
  gtk_container_add_with_properties (GTK_CONTAINER (box), label, "expand", TRUE, NULL);

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_STRING);
  g_value_set_string (&a, "IVALUE");

  g_value_unset (&a);

  return row;
}

static void
scan (GtkWidget *widget,
             OnvifPlayer *player)
{
  gchar *text;
  GtkWidget *row;
  struct DiscoveredServer server;
  struct ProbMatch m;
  int i;
  int a;

  //Clearing the list
  gtk_container_foreach (GTK_CONTAINER (player->listbox), (GtkCallback)gtk_widget_destroy, NULL);
  
  //Clearing old list data
  OnvifDeviceList__clear(player->onvifDeviceList);

  //Start UDP Scan
  server = udp_discover();
  
  printf("\nmsg uid : %s\n",server.msg_uuid);
  printf("relate uid : %s\n",server.relate_uuid);
  printf("num of match : %i\n",server.match_count);
  for (i = 0 ; i < server.match_count ; ++i) {
      m = server.matches[i];
      printf("\tProbe %s\n",m.prob_uuid);
      printf("\tAddr %s\n",m.addr);
      printf("\tTypes %s\n",m.types);
      printf("\tVersion : %s\n", m.version);
      printf("\tcount : %i\n", m.scope_count);
      for (a = 0 ; a < m.scope_count ; ++a) {
          printf("\t\tScope : %s\n",m.scopes[a]);
      }

      // Create GtkListBoxRow and add it
      row = create_row (&m,player);
      gtk_list_box_insert (GTK_LIST_BOX (player->listbox), row, -1);
  }

  //Display new content
  gtk_widget_show_all (player->listbox);
}


// gboolean update_level (OnvifPlayer* player) {
//   //TODO Compare to previous value and test
//   int d = (int) player->level;
//   gtk_level_bar_set_value (GTK_LEVEL_BAR (player->levelbar),d);
//   return TRUE;
// }

GtkWidget * create_nvt_ui (OnvifPlayer *player){
  GtkWidget *grid;
  GtkWidget *widget;

  grid = gtk_grid_new ();

  //TODO use custom drawing surface for better responsive sound level decay
  player->levelbar = gtk_level_bar_new ();
  gtk_grid_attach (GTK_GRID (grid), player->levelbar, 0, 0, 1, 1);
  gtk_level_bar_add_offset_value (GTK_LEVEL_BAR (player->levelbar),
                                  GTK_LEVEL_BAR_OFFSET_LOW,
                                  0.10);
  // alternative way to update levelbar
  // g_idle_add ((GSourceFunc) update_level, player);

  // This adds a new offset to the bar; the application will
  // be able to change its color CSS like this:
  //
  // levelbar block.my-offset {
  //   background-color: magenta;
  //   border-style: solid;
  //   border-color: black;
  //   border-style: 1px;
  // }
  gtk_level_bar_add_offset_value (GTK_LEVEL_BAR (player->levelbar), "my-offset", 0.60);

  widget = OnvifDevice__createCanvas(player);
  gtk_widget_set_vexpand (widget, TRUE);
  gtk_widget_set_hexpand (widget, TRUE);

  gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
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
  g_signal_connect (widget, "clicked", G_CALLBACK (scan), player);

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

  TITLE_STR = "Info";
  widget = gtk_frame_new (TITLE_STR);
  label = gtk_label_new (TITLE_STR);
  gtk_container_add (GTK_CONTAINER (widget), label);
  label = gtk_label_new (TITLE_STR);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, label);

  gtk_window_set_default_size(GTK_WINDOW(main_window),640,480);
  gtk_widget_show_all (main_window);
}



int main(int argc, char *argv[]) {
  OnvifPlayer data;

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Initialize our data structure */
  data = OnvifPlayer__create();
  
  /* Create the GUI */
  create_ui (&data);

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  /* Free resources */
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}
