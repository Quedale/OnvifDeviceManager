#include "player.h"

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  GError *err;
  gchar *debug_info;

  /* Print error details on the screen */
  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);

  /* Set the pipeline to READY (which stops playback) */
  gst_element_set_state (data->sink, GST_STATE_READY);
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  g_print ("End-Of-Stream reached.\n");
  gst_element_set_state (data->sink, GST_STATE_READY);
}


 GstBusSyncReply
 bus_sync_handler (GstBus * bus, GstMessage * message, OnvifPlayer * player)
 {
  // ignore anything but 'prepare-window-handle' element messages
  if (!gst_is_video_overlay_prepare_window_handle_message (message))
    return GST_BUS_PASS;

  if (player->video_window_handle != 0) {
    // GST_MESSAGE_SRC (message) will be the video sink element
    player->overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
    gst_video_overlay_set_window_handle (player->overlay, player->video_window_handle);
  } else {
    g_warning ("Should have obtained video_window_handle by now!");
  }

  gst_message_unref (message);
  return GST_BUS_DROP;
 }


gboolean level_handler(GstBus * bus, GstMessage * message, OnvifPlayer *player, GstStructure *s){

  gint channels;
  GstClockTime endtime;
  gdouble rms_dB, peak_dB, decay_dB;
  gdouble rms;
  gdouble output = 0;
  const GValue *array_val;
  const GValue *value;
  GValueArray *rms_arr, *peak_arr, *decay_arr;
  gint i;

  if (!gst_structure_get_clock_time (s, "endtime", &endtime))
    g_warning ("Could not parse endtime");

  /* the values are packed into GValueArrays with the value per channel */
  array_val = gst_structure_get_value (s, "rms");
  rms_arr = (GValueArray *) g_value_get_boxed (array_val);

  array_val = gst_structure_get_value (s, "peak");
  peak_arr = (GValueArray *) g_value_get_boxed (array_val);

  array_val = gst_structure_get_value (s, "decay");
  decay_arr = (GValueArray *) g_value_get_boxed (array_val);

  /* we can get the number of channels as the length of any of the value
    * arrays */
  channels = rms_arr->n_values;
  for (i = 0; i < channels; ++i) {
    value = g_value_array_get_nth (rms_arr, i);
    rms_dB = g_value_get_double (value);

    value = g_value_array_get_nth (peak_arr, i);
    peak_dB = g_value_get_double (value);

    value = g_value_array_get_nth (decay_arr, i);
    decay_dB = g_value_get_double (value);

    //-700 db is no sound? .. and + 100 to revert the percentag #TODO add possible positive db offset
    output = output + 100 + rms_dB / 7; 
    // g_print ("    RMS: %f dB, peak: %f dB, decay: %f dB\n",rms_dB, peak_dB, decay_dB);
    //Beep
    //-12.267482 dB, peak: -2.016216 dB, decay: -2.016216 dB
    //Quiet
    //-700.000000 dB, peak: -350.000000 dB, decay: -2.016216 dB
  }

  output = output / channels;

  int intv = (int) output;
  if( intv == player->level){
    //Ignore
  } else if( intv > player->level){
    //Set new peak
    gtk_level_bar_set_value (GTK_LEVEL_BAR (player->levelbar),output/100);
    player->level = (int *) intv;
  } else {
    int variance = player->level-intv;
    if(variance > 15){
      int tmpv = (int) player->level;
      tmpv=tmpv-15;
      player->level= tmpv;
    } else {
      player->level = intv;
    }
    int tmpv2 = (int) player->level;
    double toset = (double) tmpv2/(double)100;
    gtk_level_bar_set_value (GTK_LEVEL_BAR (player->levelbar),toset);
  }


  return TRUE;

}

int
message_handler (GstBus * bus, GstMessage * message, gpointer p)
{ 
  OnvifPlayer *player = (OnvifPlayer *) p;
  switch(message->type){
    case GST_MESSAGE_UNKNOWN:
      printf("msg : GST_MESSAGE_UNKNOWN\n");
      break;
    case GST_MESSAGE_EOS:
      error_cb(bus,message,player);
      break;
    case GST_MESSAGE_ERROR:
      eos_cb(bus,message,player);
      break;
    case GST_MESSAGE_WARNING:
      printf("msg : GST_MESSAGE_WARNING\n");
      break;
    case GST_MESSAGE_INFO:
      printf("msg : GST_MESSAGE_INFO\n");
      break;
    case GST_MESSAGE_TAG:
      // tag_cb(bus,message,player);
      break;
    case GST_MESSAGE_BUFFERING:
      printf("msg : GST_MESSAGE_BUFFERING\n");
      break;
    case GST_MESSAGE_STATE_CHANGED:
      // state_changed_cb(bus,message,player);
      break;
    case GST_MESSAGE_STATE_DIRTY:
      printf("msg : GST_MESSAGE_STATE_DIRTY\n");
      break;
    case GST_MESSAGE_STEP_DONE:
      printf("msg : GST_MESSAGE_STEP_DONE\n");
      break;
    case GST_MESSAGE_CLOCK_PROVIDE:
      printf("msg : GST_MESSAGE_CLOCK_PROVIDE\n");
      break;
    case GST_MESSAGE_CLOCK_LOST:
      printf("msg : GST_MESSAGE_CLOCK_LOST\n");
      break;
    case GST_MESSAGE_NEW_CLOCK:
      break;
    case GST_MESSAGE_STRUCTURE_CHANGE:
      printf("msg : GST_MESSAGE_STRUCTURE_CHANGE\n");
      break;
    case GST_MESSAGE_STREAM_STATUS:
      break;
    case GST_MESSAGE_APPLICATION:
      printf("msg : GST_MESSAGE_APPLICATION\n");
      break;
    case GST_MESSAGE_ELEMENT:
      GstStructure *s = gst_message_get_structure (message);
      gchar *name = gst_structure_get_name (s);

      if (strcmp (name, "level") == 0) {
        return level_handler(bus,message,player,s);
      } else if (strcmp (name, "GstNavigationMessage") == 0){
        //Ignore intentionally left unhandled for now
      } else {
        printf("TODO Unhandled element msg name : %s//%d\n",name,message->type);
      }
      break;
    case GST_MESSAGE_SEGMENT_START:
      printf("msg : GST_MESSAGE_SEGMENT_START\n");
      break;
    case GST_MESSAGE_SEGMENT_DONE:
      printf("msg : GST_MESSAGE_SEGMENT_DONE\n");
      break;
    case GST_MESSAGE_DURATION_CHANGED:
      printf("msg : GST_MESSAGE_DURATION_CHANGED\n");
      break;
    case GST_MESSAGE_LATENCY:
      break;
    case GST_MESSAGE_ASYNC_START:
      printf("msg : GST_MESSAGE_ASYNC_START\n");
      break;
    case GST_MESSAGE_ASYNC_DONE:
      break;
    case GST_MESSAGE_REQUEST_STATE:
      printf("msg : GST_MESSAGE_REQUEST_STATE\n");
      break;
    case GST_MESSAGE_STEP_START:
      printf("msg : GST_MESSAGE_STEP_START\n");
      break;
    case GST_MESSAGE_QOS:
      break;
    case GST_MESSAGE_PROGRESS:
      break;
    case GST_MESSAGE_TOC:
      printf("msg : GST_MESSAGE_TOC\n");
      break;
    case GST_MESSAGE_RESET_TIME:
      printf("msg : GST_MESSAGE_RESET_TIME\n");
      break;
    case GST_MESSAGE_STREAM_START:
      break;
    case GST_MESSAGE_NEED_CONTEXT:
      break;
    case GST_MESSAGE_HAVE_CONTEXT:
      break;
    case GST_MESSAGE_EXTENDED:
      printf("msg : GST_MESSAGE_EXTENDED\n");
      break;
    case GST_MESSAGE_DEVICE_ADDED:
      printf("msg : GST_MESSAGE_DEVICE_ADDED\n");
      break;
    case GST_MESSAGE_DEVICE_REMOVED:
      printf("msg : GST_MESSAGE_DEVICE_REMOVED\n");
      break;
    case GST_MESSAGE_PROPERTY_NOTIFY:
      printf("msg : GST_MESSAGE_PROPERTY_NOTIFY\n");
      break;
    case GST_MESSAGE_STREAM_COLLECTION:
      printf("msg : GST_MESSAGE_STREAM_COLLECTION\n");
      break;
    case GST_MESSAGE_STREAMS_SELECTED:
      printf("msg : GST_MESSAGE_STREAMS_SELECTED\n");
      break;
    case GST_MESSAGE_REDIRECT:
      printf("msg : GST_MESSAGE_REDIRECT\n");
      break;
    case GST_MESSAGE_DEVICE_CHANGED:
      printf("msg : GST_MESSAGE_DEVICE_CHANGED\n");
      break;
    case GST_MESSAGE_INSTANT_RATE_REQUEST:
      printf("msg : GST_MESSAGE_INSTANT_RATE_REQUEST\n");
      break;
    case GST_MESSAGE_ANY:
      printf("msg : GST_MESSAGE_ANY\n");
      break;
    default:
      printf("msg : default....");

  }

  return TRUE;
}

/* This function is called when the GUI toolkit creates the physical window that will hold the video.
 * At this point we can retrieve its handler (which has a different meaning depending on the windowing system)
 * and pass it to GStreamer through the VideoOverlay interface. */
static void realize_cb (GtkWidget *widget, OnvifPlayer *data) {
  GdkWindow *window = gtk_widget_get_window (widget);

  if (!gdk_window_ensure_native (window))
    g_error ("Couldn't create native window needed for GstVideoOverlay!");

  /* Retrieve window handler from GDK */
#if defined (GDK_WINDOWING_WIN32)
  data->video_window_handle = (guintptr)GDK_WINDOW_HWND (window);
#elif defined (GDK_WINDOWING_QUARTZ)
  data->video_window_handle = gdk_quartz_window_get_nsview (window);
#elif defined (GDK_WINDOWING_X11)
  data->video_window_handle = GDK_WINDOW_XID (window);
#endif
  // //Register bus signal watch to set GstVideoOver now that we have the video handle
  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (data->pipeline));
  gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, data,NULL);
  //TODO clean up watch
  //Registering message_handler after realize since nested pointers will be assigned after creation
  guint watch_id = gst_bus_add_watch (bus, message_handler, data);
  gst_object_unref (bus);

}

/* This function is called everytime the video window needs to be redrawn (due to damage/exposure,
 * rescaling, etc). GStreamer takes care of this in the PAUSED and PLAYING states, otherwise,
 * we simply draw a black rectangle to avoid garbage showing up. */
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, OnvifPlayer *data) {
  if (data->state < GST_STATE_PAUSED) {
    GtkAllocation allocation;

    /* Cairo is a 2D graphics library which we use here to clean the video window.
     * It is used by GStreamer for other reasons, so it will always be available to us. */
    gtk_widget_get_allocation (widget, &allocation);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    cairo_fill (cr);
  }

  return FALSE;
}

//TODO Create pipeline manually selecting elements for faster construct
void * create_pipeline(OnvifPlayer *self){
  GstElement *lvler;
  self->pipeline = gst_parse_launch ("rtspsrc backchannel=onvif debug=true name=r "
      "r. ! queue ! decodebin ! queue ! autovideosink name=vsink " //glimagesink gives warning
      "r. ! queue ! decodebin ! queue ! audioconvert ! level name=lvler post-messages=true messages=true ! autoaudiosink ", NULL);

    self->src = gst_bin_get_by_name (GST_BIN (self->pipeline), "r");
    if (!self->src) {
      g_printerr ("Not all elements could be created.\n");
      return NULL;
    }

    self->sink = gst_bin_get_by_name (GST_BIN (self->pipeline), "vsink");
    if (!self->sink) {
        g_printerr ("Not all elements could be created.\n");
        return NULL;
    }
    
    lvler = gst_bin_get_by_name (GST_BIN (self->pipeline), "lvler");
    if (!lvler) {
        g_printerr ("Not all elements could be created.\n");
        return NULL;
    }
}

void OnvifPlayer__init(OnvifPlayer* self) {

    self->video_window_handle = 0;
    self->level = malloc(sizeof(double));
    self->level = 0;
    self->onvifDeviceList = OnvifDeviceList__create();
    self->levelbar =  (GtkWidget *) malloc(sizeof(GtkWidget));
    create_pipeline(self);
    if (!self->pipeline){
      g_error ("Failed to parse pipeline");
      return;
    }

 }

OnvifPlayer OnvifPlayer__create() {
    OnvifPlayer result;
    memset (&result, 0, sizeof (result));
    OnvifPlayer__init(&result);
    return result;
}

void OnvifPlayer__reset(OnvifPlayer* self) {
}

void OnvifPlayer__destroy(OnvifPlayer* self) {
  if (self) {
     OnvifPlayer__reset(self);
     free(self);
  }
}

void OnvifPlayer__set_playback_url(OnvifPlayer* self, char *url) {
    // g_object_set (self->sink, "uri", url, NULL);
    printf("set location : %s\n",url);
    g_object_set (G_OBJECT (self->src), "location", url, NULL);
}

void OnvifPlayer__play(OnvifPlayer* self){
    GstStateChangeReturn ret;
    ret = gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (self->pipeline);
        return;
    }
    printf("set state to playing..\n");
}

GtkWidget * OnvifDevice__createCanvas(OnvifPlayer *self){
  GtkWidget *widget;
  widget = gtk_drawing_area_new ();
  gtk_widget_set_double_buffered (widget, FALSE);
  g_signal_connect (widget, "realize", G_CALLBACK (realize_cb), self);
  g_signal_connect (widget, "draw", G_CALLBACK (draw_cb), self);
  
  return widget;
}