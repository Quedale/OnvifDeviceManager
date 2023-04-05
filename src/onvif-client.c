/*
 * 
 * Utility design to test various pipeline and elements
 * Useful to test pipeline re-usability
 *  
 *  ./onvifclient rtsp://localhost:8556/test rtsp://remote:8554/test
 * 
 */ 


#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include "gst/gtk/gstgtkbasesink.h"
#include "gst/onvifinitstaticplugins.h"
#include <gtk/gtk.h>
#include<unistd.h>

typedef struct {
  GstElement * pipeline;
  GstElement * sink;
  GstElement *rtspsrc;
  GtkWidget * canvas;
  GtkWidget * container;
  int url_count;
  int current_url_index;
  char ** urls;
} AppData;


void
create_pipeline (AppData * data)
{
  // data->pipeline = gst_parse_launch ("rtspsrc buffer-mode=0 do-retransmission=false is-live=true teardown-timeout=0 name=r "
  //     " ! rtph264depay ! h264parse name=parser ! avdec_h264 name=decoder ! videoconvert ! ximagesink "
  data->pipeline = gst_parse_launch ("rtspsrc name=r teardown-timeout=0 ! rtph264depay ! h264parse ! openh264dec ! videoconvert ! gtkcustomsink name=sink"
      , NULL);
  // data->pipeline = gst_parse_launch ("videotestsrc ! ximagesink"
  //     , NULL);
  if (!data->pipeline)
    g_error ("Failed to parse pipeline");

  data->rtspsrc = gst_bin_get_by_name (GST_BIN (data->pipeline), "r");
  data->sink = gst_bin_get_by_name (GST_BIN (data->pipeline), "sink");
}

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, AppData * appdata) {
  gst_element_set_state (appdata->pipeline, GST_STATE_READY);
  gst_element_set_state (appdata->pipeline, GST_STATE_NULL);
  gtk_main_quit ();
}

/* This function is called when the STOP button is clicked */
static void stop_cb (GtkButton *button, AppData * appdata) {
  gst_element_set_state (appdata->pipeline, GST_STATE_NULL);

  appdata->canvas = gst_gtk_base_custom_sink_acquire_widget(GST_GTK_BASE_CUSTOM_SINK(appdata->sink));
  
  gst_object_unref (appdata->pipeline);
  
  create_pipeline(appdata);
}

/* This function is called when the STOP button is clicked */
static void play_cb (GtkButton *button, AppData * appdata) {
  stop_cb(button,appdata);
  gst_gtk_base_custom_sink_set_parent(GST_GTK_BASE_CUSTOM_SINK(appdata->sink),appdata->container);
  gst_gtk_base_custom_sink_set_widget(GST_GTK_BASE_CUSTOM_SINK(appdata->sink),GTK_GST_BASE_CUSTOM_WIDGET(appdata->canvas));
  if(appdata->current_url_index == appdata->url_count-1){
    appdata->current_url_index = 1;
  } else {
    appdata->current_url_index++;
  }

  printf("playing %s\n",appdata->urls[appdata->current_url_index]);
  g_object_set (G_OBJECT (appdata->rtspsrc), "location", appdata->urls[appdata->current_url_index], NULL);
  gst_element_set_state (appdata->pipeline, GST_STATE_PLAYING);
}


void
create_control_window(AppData * appdata){
  GtkWidget * main_window, *widget, *grid;

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);  
  g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), appdata);

  gtk_window_set_title (GTK_WINDOW (main_window), "Gstreamer Controls");

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 20);

  gtk_container_add(GTK_CONTAINER(main_window),grid);

  widget = gtk_button_new_with_label ("Play Next");
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);
  g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK (play_cb), appdata);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);
  
  widget = gtk_button_new_with_label ("Stop");
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);
  g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK (stop_cb), appdata);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 1);

  appdata->container  = gtk_grid_new();
  gtk_grid_attach (GTK_GRID (grid), appdata->container, 0, 1, 2, 1);

  gtk_window_set_default_size(GTK_WINDOW(main_window),400,300);
  gtk_widget_show_all (main_window);
}

char **copy_argv(int argc, char *argv[]) {
  // calculate the contiguous argv buffer size
  int length=0;
  size_t ptr_args = argc + 1;
  for (int i = 0; i < argc; i++)
  {
    length += (strlen(argv[i]) + 1);
  }
  char** new_argv = (char**)malloc((ptr_args) * sizeof(char*) + length);
  // copy argv into the contiguous buffer
  length = 0;
  for (int i = 0; i < argc; i++)
  {
    new_argv[i] = &(((char*)new_argv)[(ptr_args * sizeof(char*)) + length]);
    strcpy(new_argv[i], argv[i]);
    length += (strlen(argv[i]) + 1);
  }
  // insert NULL terminating ptr at the end of the ptr array
  new_argv[ptr_args-1] = NULL;
  return (new_argv);
}

int
main (int argc, char *argv[])
{
  /* Initialize Gstreamer and static plugins */
  gst_init (&argc, &argv);
  onvif_init_static_plugins();

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  if (argc < 1) {
    printf("error : Two parameters rtsp url are required.");
    return 1;
  }

  /* Initialize Application data structure */
  AppData * data = malloc(sizeof(AppData));
  data->url_count = argc;
  data->urls = copy_argv(argc,argv);
  data->pipeline = NULL;
  data->current_url_index = 0;

  create_pipeline(data);


  create_control_window(data);

  gtk_main ();
  
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
  gst_object_unref (data->pipeline);
  free(data);
}
