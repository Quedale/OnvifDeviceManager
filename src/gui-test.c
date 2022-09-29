#include <gtk/gtk.h>
#include "udp/soap_parser.h"
#include "udp/discovery.h"
#include "soap/client.h"
#include "ui/onvif_list.h"

static GtkTargetEntry entries[] = {
  { "GTK_LIST_BOX_ROW", GTK_TARGET_SAME_APP, 0 }
};

/*
 * listitem_selected
 *
 * Event handler called when an item is selected.
 */
void listitem_selected (GtkWidget *widget,   GtkListBoxRow* row,
  gpointer user_data)
{
    int* number;
    number = (int*)user_data;
    printf("Number: %ls\n", number);
    
    int pos;
    pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
    g_print ("item selected - %i\n", pos);
    
}

static void
drag_begin (GtkWidget      *widget,
            GdkDragContext *context,
            gpointer        data)
{
  GtkWidget *row;
  GtkAllocation alloc;
  cairo_surface_t *surface;
  cairo_t *cr;
  int x, y;
  double sx, sy;

  row = gtk_widget_get_ancestor (widget, GTK_TYPE_LIST_BOX_ROW);
  gtk_widget_get_allocation (row, &alloc);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, alloc.width, alloc.height);
  cr = cairo_create (surface);

  gtk_style_context_add_class (gtk_widget_get_style_context (row), "drag-icon");
  gtk_widget_draw (row, cr);
  gtk_style_context_remove_class (gtk_widget_get_style_context (row), "drag-icon");

  gtk_widget_translate_coordinates (widget, row, 0, 0, &x, &y);
  cairo_surface_get_device_scale (surface, &sx, &sy);
  cairo_surface_set_device_offset (surface, -x * sx, -y * sy);
  gtk_drag_set_icon_surface (context, surface);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

void
drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *selection_data,
               guint             info,
               guint             time,
               gpointer          data)
{
  gtk_selection_data_set (selection_data,
                          gdk_atom_intern_static_string ("GTK_LIST_BOX_ROW"),
                          32,
                          (const guchar *)&widget,
                          sizeof (gpointer));
}

static void
drag_data_received (GtkWidget        *widget,
                    GdkDragContext   *context,
                    gint              x,
                    gint              y,
                    GtkSelectionData *selection_data,
                    guint             info,
                    guint32           time,
                    gpointer          data)
{
  GtkWidget *target;
  GtkWidget *row;
  GtkWidget *source;
  int pos;

  target = widget;

  pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (target));
  row = (gpointer)* (gpointer*)gtk_selection_data_get_data (selection_data);
  source = gtk_widget_get_ancestor (row, GTK_TYPE_LIST_BOX_ROW);

  if (source == target)
    return;

  g_object_ref (source);
  gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (source)), source);
  gtk_list_box_insert (GTK_LIST_BOX (gtk_widget_get_parent (target)), source, pos);
  g_object_unref (source);
}

GtkWidget *listbox;
OnvifDeviceList* onvifDeviceList; //List kept in sync with list box.

static GtkWidget *
create_row (struct ProbMatch * m)
{
  GtkWidget *row, *handle, *box, *label, *image;
  OnvifSoapClient* client = OnvifSoapClient__create(m->addr);
  char *hostname = OnvifSoapClient__getHostname(client);
  OnvifDevice dev;
  dev.hostname = hostname;
  
  OnvifDeviceList__insert_element(onvifDeviceList,dev,onvifDeviceList->device_count);
  int b;
  for (b=0;b<onvifDeviceList->device_count;b++){
    printf("DEBUG List Record :[%i] %s\n",b,onvifDeviceList->devices[b].hostname);
  }

  row = gtk_list_box_row_new ();

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  gtk_container_add (GTK_CONTAINER (row), box);

  handle = gtk_event_box_new ();
  image = gtk_image_new_from_icon_name ("open-menu-symbolic", 1);
  gtk_container_add (GTK_CONTAINER (handle), image);
  gtk_container_add (GTK_CONTAINER (box), handle);

  label = gtk_label_new (hostname);
  gtk_container_add_with_properties (GTK_CONTAINER (box), label, "expand", TRUE, NULL);

  gtk_drag_source_set (handle, GDK_BUTTON1_MASK, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (handle, "drag-begin", G_CALLBACK (drag_begin), NULL);
  g_signal_connect (handle, "drag-data-get", G_CALLBACK (drag_data_get), NULL);

  gtk_drag_dest_set (row, GTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (row, "drag-data-received", G_CALLBACK (drag_data_received), NULL);

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_STRING);
  g_value_set_string (&a, "IVALUE");

  g_value_unset (&a);

  return row;
}

static void
scan (GtkWidget *widget,
             gpointer   data)
{
  gchar *text;
  GtkWidget *row;
  struct DiscoveredServer server;
  struct ProbMatch m;
  int i;
  int a;

  //Clearing the list
  gtk_container_foreach (GTK_CONTAINER (listbox), (GtkCallback)gtk_widget_destroy, NULL);
  
  //Clearing old list data
  OnvifDeviceList__clear(onvifDeviceList);
  
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

      //Create GtkListBoxRow and add it
      row = create_row (&m);
      gtk_list_box_insert (GTK_LIST_BOX (listbox), row, -1);
  }

  //Display new content
  gtk_widget_show_all (listbox);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *frame;

  /* create a new window, and set its title */
  window = gtk_application_window_new (app);
  gtk_window_set_default_size(GTK_WINDOW(window),640,480);

  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  /* Here we construct the container that is going pack our buttons */
  grid = gtk_grid_new ();
  // g_object_set (grid, "expand", FALSE, NULL);

  gtk_container_add (GTK_CONTAINER (window), grid);


  button = gtk_button_new ();
  label = gtk_label_new("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\">Scan</span>...");
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_widget_set_vexpand (button, FALSE);
  gtk_widget_set_hexpand (button, FALSE);
  gtk_widget_set_size_request(button,-1,80);
  g_signal_connect (button, "clicked", G_CALLBACK (scan), NULL);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);


  /* --- Create a list item from the data element --- */
  listbox = gtk_list_box_new ();
  gtk_widget_set_size_request(listbox,100,-1);
  gtk_widget_set_vexpand (listbox, TRUE);
  gtk_widget_set_hexpand (listbox, FALSE);
  g_signal_connect_swapped (listbox, "row-selected", G_CALLBACK (listitem_selected), NULL);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox), GTK_SELECTION_SINGLE);
  gtk_grid_attach (GTK_GRID (grid), listbox, 0, 1, 1, 1);

  button = gtk_button_new_with_label ("Quit");
  gtk_widget_set_vexpand (button, FALSE);
  gtk_widget_set_hexpand (button, FALSE);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 1, 1);

  //Padding (TODO Implement draggable resizer)
  button = gtk_label_new("");
  gtk_widget_set_vexpand (button, TRUE);
  gtk_widget_set_hexpand (button, FALSE);
  gtk_widget_set_size_request(button,20,-1);
  gtk_grid_attach (GTK_GRID (grid), button, 1, 0, 1, 3);

  // button = gtk_widget_new (GTK_TYPE_LABEL, "label", "Hello World", "xalign", 0.0, NULL);
  button = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (button), GTK_POS_TOP);

  gtk_widget_set_vexpand (button, TRUE);
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 2, 0, 1, 3);

  char bufferf[32];
  char bufferl[32];
  int i;
  /* lets append a bunch of pages to the notebook */
  for (i=0; i < 5; i++) {
      sprintf(bufferf, "Append Frame %d", i+1);
      sprintf(bufferl, "Page %d", i+1);
      
      frame = gtk_frame_new (bufferf);
      
      label = gtk_label_new (bufferf);
      gtk_container_add (GTK_CONTAINER (frame), label);
      gtk_widget_show (label);
      
      label = gtk_label_new (bufferl);
      gtk_notebook_append_page (GTK_NOTEBOOK (button), frame, label);
  }

  gtk_widget_show_all (window);

}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;
  onvifDeviceList = OnvifDeviceList__create();

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
