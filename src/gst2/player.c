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


gboolean level_handler(GstBus * bus, GstMessage * message, OnvifPlayer *player, const GstStructure *s){

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

//No control over the use of GValueArray over GArray since its constructed by gstreamer libs
//Depracation can't be fixed until Gstreamer update this library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
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
#pragma GCC diagnostic pop
  output = output / channels;
  double intv = (double)output;
  if( intv == player->level){
    //Ignore
  } else if( intv > player->level){
    //Set new peak
    gtk_level_bar_set_value (GTK_LEVEL_BAR (player->levelbar),output/100);
    player->level = intv;
  } else {
    double variance = player->level-intv;
    if(variance > 15){
      player->level=player->level-15;
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
      GError *gerror;
      gchar *debug;

      if (message->type == GST_MESSAGE_ERROR)
        gst_message_parse_error (message, &gerror, &debug);
      else
        gst_message_parse_warning (message, &gerror, &debug);

      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
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
      const GstStructure *s = gst_message_get_structure (message);
      const gchar *name = gst_structure_get_name (s);

      if (strcmp (name, "level") == 0) {
        return level_handler(bus,message,player,s);
      } else if (strcmp (name, "GstNavigationMessage") == 0 || 
                strcmp (name, "application/x-rtp-source-sdes") == 0){
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

gboolean
remove_extra_fields (GQuark field_id, GValue * value G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED)
{
  return !g_str_has_prefix (g_quark_to_string (field_id), "a-");
}

GstFlowReturn
new_sample (GstElement * appsink, OnvifPlayer * player)
{
  GstSample *sample;
  GstFlowReturn ret = GST_FLOW_OK;

  g_assert (player->back_stream_id != (guint *)-1);

  g_signal_emit_by_name (appsink, "pull-sample", &sample);

  if (!sample)
    goto out;

  g_signal_emit_by_name (player->src, "push-backchannel-buffer", player->back_stream_id, sample, &ret);

out:
  return ret;
}

void
setup_backchannel_shoveler (OnvifPlayer * player, GstCaps * caps)
{
  GstElement *appsink;
  GstElement *backpipe;
  
  backpipe = gst_parse_launch ("audiotestsrc is-live=true wave=red-noise ! "
      "mulawenc ! rtppcmupay ! appsink name=out", NULL);
  if (!backpipe)
    g_error ("Could not setup backchannel pipeline");

  appsink = gst_bin_get_by_name (GST_BIN (backpipe), "out");
  g_object_set (G_OBJECT (appsink), "caps", caps, "emit-signals", TRUE, NULL);

  g_signal_connect (appsink, "new-sample", G_CALLBACK (new_sample), player);

  g_print ("Playing backchannel shoveler\n");
  gst_element_set_state (backpipe, GST_STATE_PLAYING);
}

gboolean
find_backchannel (GstElement * rtspsrc, guint idx, GstCaps * caps,
    OnvifPlayer *player G_GNUC_UNUSED)
{
  GstStructure *s;
  gchar *caps_str = gst_caps_to_string (caps);
  g_print ("Selecting stream idx %u, caps %s\n", idx, caps_str);
  g_free (caps_str);

  s = gst_caps_get_structure (caps, 0);
  if (gst_structure_has_field (s, "a-sendonly")) {
    player->back_stream_id = (guint *) &idx;
    caps = gst_caps_new_empty ();
    s = gst_structure_copy (s);
    gst_structure_set_name (s, "application/x-rtp");
    gst_structure_filter_and_map_in_place (s, remove_extra_fields, NULL);
    gst_caps_append_structure (caps, s);
    setup_backchannel_shoveler (player, caps);
  }

  return TRUE;
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
    self->back_stream_id = malloc(sizeof(guint));
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  //TODO Support Gl. Currently not well supported by WSL
  gtk_widget_set_double_buffered (widget, FALSE);
#pragma GCC diagnostic pop
  g_signal_connect (widget, "realize", G_CALLBACK (realize_cb), self);
  g_signal_connect (widget, "draw", G_CALLBACK (draw_cb), self);
  g_signal_connect (self->src, "select-stream", G_CALLBACK (find_backchannel),self);
  return widget;
}