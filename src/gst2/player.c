#include "player.h"
#include <unistd.h>

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
  if(output == player->level){
    //Ignore
  } else if( output > player->level){
    //Set new peak
    player->level = output;
  } else {
    //Lower to a maximum drop of 15 for a graphical decay
    gdouble variance = player->level-output;
    if(variance > 7.5){
      player->level=player->level-7.5; //The decay value has to match the interval set. We are dealing with 2x faster interval therefor 15/2=7.5
    } else {
      player->level = output;
    }
  }
  return TRUE;

}


/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void
state_changed_cb (GstBus * bus, GstMessage * msg, OnvifPlayer * data)
{
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
    data->state = new_state;
    if(data->state == GST_STATE_PLAYING){
      gtk_widget_set_visible(data->canvas, TRUE);
    } else {
      gtk_widget_set_visible(data->canvas, FALSE);
    }
    g_print ("State set to %s\n", gst_element_state_get_name (new_state));
  }
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
      state_changed_cb(bus,message,player);
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

  g_assert (player->back_stream_id != -1);

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
  
  player->backpipe = gst_parse_launch ("audiotestsrc is-live=true wave=red-noise ! "
      "mulawenc ! rtppcmupay ! appsink name=out", NULL);
  if (!player->backpipe)
    g_error ("Could not setup backchannel pipeline");

  appsink = gst_bin_get_by_name (GST_BIN (player->backpipe), "out");
  g_object_set (G_OBJECT (appsink), "caps", caps, "emit-signals", TRUE, NULL);

  g_signal_connect (appsink, "new-sample", G_CALLBACK (new_sample), player);

  g_print ("Playing backchannel shoveler\n");
  gst_element_set_state (player->backpipe, GST_STATE_PLAYING);
}

gboolean
find_backchannel (GstElement * rtspsrc, guint idx, GstCaps * caps,
    OnvifPlayer *player G_GNUC_UNUSED)
{
  GstStructure *s;
  gchar *caps_str = gst_caps_to_string (caps);
  g_free (caps_str);

  s = gst_caps_get_structure (caps, 0);
  if (gst_structure_has_field (s, "a-sendonly")) {
    player->back_stream_id = idx;
    caps = gst_caps_new_empty ();
    s = gst_structure_copy (s);
    gst_structure_set_name (s, "application/x-rtp");
    gst_structure_filter_and_map_in_place (s, remove_extra_fields, NULL);
    gst_caps_append_structure (caps, s);
    setup_backchannel_shoveler (player, caps);
  }

  return TRUE;
}

/* Dynamically link */
static void on_pad_added (GstElement *element, GstPad *pad, gpointer data){
    GstPad *sinkpad;
    GstPadLinkReturn ret;
    GstElement *decoder = (GstElement *) data;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;
    gchar *capstr = NULL;
    //TODO Check caps before linking (audio to vidoe and video to audio invalid linking)
    /* We can now link this pad with the rtsp-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    ret = gst_pad_link (pad, sinkpad);

    if (GST_PAD_LINK_FAILED (ret)) {
      g_print("failed to link dynamically\n");
    }

    gst_object_unref (sinkpad);
}

static void
prepare_overlay (GstElement * overlay, GstCaps * caps, gpointer user_data)
{
  OnvifPlayer *player = (OnvifPlayer *)user_data;
  GstVideoInfo* video_info = gst_video_info_new();
  if(!gst_video_info_from_caps(video_info,caps)){
    g_print("failed to parse caps\n");
    return;
  }

  player->width = video_info->width;
  player->height = video_info->height;
  gst_video_info_free(video_info);
}

static void
draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp, 
  guint64 duration, gpointer user_data)
{
  OnvifPlayer *player = (OnvifPlayer *)user_data;
  double scale;

  gdouble level = player->level;
  gdouble margin = 10;
  gdouble bwidth = 20;
  // gdouble level = 100;
  gdouble pheight = player->height - margin*2;
  //Dont bother with the overlay if it's not visible
  if(player->height == 0 || player->width == 0)
    return;

  gdouble bheight = level * pheight / 100;
  gdouble diff = pheight - bheight;
  cairo_rectangle(cr, player->width-(margin+bwidth), margin+diff, bwidth, bheight);
  cairo_set_source_rgba (cr, 0.9, 0.0, 0.1, 0.7);
  cairo_fill(cr);

}

void * create_pipeline(OnvifPlayer *self){
  GstElement *rtph264depay;
  GstElement *h264parse;
  GstElement *vdecoder;
  GstElement *videoqueue0;
  GstElement *videoconvert;
  GstElement *cairo_overlay;
  GstElement *adecoder;
  GstElement *audioqueue0;
  GstElement *audioconvert;
  GstElement *level;
  GstElement *audio_sink;

  /* Create the elements */
  self->src = gst_element_factory_make ("rtspsrc", "src");
  //Video pipe
  rtph264depay = gst_element_factory_make ("rtph264depay", "rtph264depay0");
  h264parse = gst_element_factory_make ("h264parse", "h264parse0");
  vdecoder = gst_element_factory_make ("decodebin", "decodebin0");
  videoqueue0 = gst_element_factory_make ("queue", "videoqueue0");
  videoconvert = gst_element_factory_make ("videoconvert", "videoconvert0");
  cairo_overlay = gst_element_factory_make ("cairooverlay", "overlay");
  self->sink = gst_element_factory_make ("autovideosink", "vsink");
  g_object_set (G_OBJECT (self->sink), "sync", FALSE, NULL);
  g_object_set (G_OBJECT (self->src), "latency", 0, NULL);
  g_object_set (G_OBJECT (self->src), "teardown-timeout", 0, NULL); 
  g_object_set (G_OBJECT (self->src), "backchannel", 1, NULL);
  g_object_set (G_OBJECT (self->src), "user-agent", "OnvifDeviceManager-Linux-0.0", NULL);
  g_object_set (G_OBJECT (self->src), "do-retransmission", FALSE, NULL);
  g_object_set (G_OBJECT (self->src), "onvif-mode", TRUE, NULL);

  //Audio pipe
  adecoder = gst_element_factory_make ("decodebin", "decodebin1");
  audioqueue0 = gst_element_factory_make ("queue", "audioqueue0");
  audioconvert = gst_element_factory_make ("audioconvert", "audioconvert0");
  level = gst_element_factory_make("level","level0");
  audio_sink = gst_element_factory_make ("autoaudiosink", "autoaudiosink0");
  g_object_set (G_OBJECT (audio_sink), "sync", FALSE, NULL);
  g_object_set (G_OBJECT (level), "post-messages", TRUE, NULL);
  //For smoother responsiveness and more accurate value, lowered interval by 2x
  g_object_set (G_OBJECT (level), "interval", 50000000, NULL);

  /* Create the empty pipeline */
  self->pipeline = malloc(sizeof(GstElement *));
  self->pipeline = gst_pipeline_new ("test-pipeline");

  //Make sure: Every elements was created ok
  if (!self->pipeline || !self->src || !rtph264depay || !h264parse || !vdecoder || !videoqueue0 || !videoconvert || !cairo_overlay || !self->sink || !adecoder || !audioqueue0 || !audioconvert || !level || !audio_sink) {
      g_printerr ("One of the elements wasn't created... Exiting\n");
      return NULL;
  }

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->pipeline), self->src, rtph264depay, h264parse, vdecoder, videoqueue0, videoconvert, cairo_overlay, self->sink, adecoder, audioqueue0, audioconvert, level, audio_sink, NULL);

  // Link confirmation
  if (!gst_element_link_many (rtph264depay, h264parse, vdecoder, NULL)){
      g_warning ("Linking part (A)-1 Fail...\n");
      return NULL;
  }

  // Link confirmation
  if (!gst_element_link_many (videoqueue0, videoconvert, cairo_overlay, self->sink, NULL)){
      g_warning ("Linking part (A)-2 Fail...");
      return NULL;
  }

  // Link confirmation
  if (!gst_element_link_many (audioqueue0, audioconvert, level, audio_sink, NULL)){
      g_warning ("Linking part (B)-2 Fail...");
      return NULL;
  }

  // Dynamic Pad Creation
  if(! g_signal_connect (self->src, "pad-added", G_CALLBACK (on_pad_added),rtph264depay))
  {
      g_warning ("Linking part (1) with part (A)-1 Fail...");
  }

  // Dynamic Pad Creation
  if(! g_signal_connect (vdecoder, "pad-added", G_CALLBACK (on_pad_added),videoqueue0))
  {
      g_warning ("Linking part (2) with part (A)-2 Fail...");
  }

  // Dynamic Pad Creation
  if(! g_signal_connect (self->src, "pad-added", G_CALLBACK (on_pad_added),adecoder))
  {
      g_warning ("Linking part (1) with part (B)-1 Fail...");
  }

   // Dynamic Pad Creation
  if(! g_signal_connect (adecoder, "pad-added", G_CALLBACK (on_pad_added),audioqueue0))
  {
      g_warning ("Linking part (2) with part (B)-2 Fail...");
  }
   

 // Dynamic Pad Creation
  if(! g_signal_connect (cairo_overlay, "draw", G_CALLBACK (draw_overlay), self))
  {
      g_warning ("cairo_overlay draw callback Fail...");
  }
 // Dynamic Pad Creation
  if(! g_signal_connect (cairo_overlay, "caps-changed", G_CALLBACK (prepare_overlay), self))
  {
      g_warning ("cairo_overlay caps-changed   callback Fail...");
  }

}

void OnvifPlayer__init(OnvifPlayer* self) {

    self->video_window_handle = 0;
    self->level = 0;
    self->onvifDeviceList = OnvifDeviceList__create();
    self->width = 0;
    self->height = 0;
    self->device = NULL;
    self->player_lock =malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(self->player_lock, NULL);
    create_pipeline(self);
    if (!self->pipeline){
      g_error ("Failed to parse pipeline");
      return;
    }

 }

OnvifPlayer * OnvifPlayer__create() {
    OnvifPlayer *result  =  (OnvifPlayer *) malloc(sizeof(OnvifPlayer));
    OnvifPlayer__init(result);
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
    printf("set location : %s\n",url);
    g_object_set (G_OBJECT (self->src), "location", url, NULL);
}

void OnvifPlayer__stop(OnvifPlayer* self){
    printf("OnvifPlayer__stop \n");
    if(self->state <= GST_STATE_READY){
      return; //Ignore stop call if not playing
    }
    
    pthread_mutex_lock(self->player_lock);
    GstStateChangeReturn ret;
    ret = gst_element_set_state (self->pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      g_printerr ("Unable to set the pipeline to the ready state.\n");
      gst_object_unref (self->pipeline);
      pthread_mutex_unlock(self->player_lock);
      return;
    }

    //Backchannel clean up
    if(GST_IS_ELEMENT(self->backpipe)){
      ret = gst_element_set_state (self->backpipe, GST_STATE_READY);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          g_printerr ("Unable to set the backpipe to the ready state.\n");
          gst_object_unref (self->pipeline);
          pthread_mutex_unlock(self->player_lock);
          return;
      }
      ret = gst_element_set_state (self->backpipe, GST_STATE_NULL);
      gst_object_unref (self->backpipe);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          g_printerr ("Unable to set the backpipe to the ready state.\n");
          gst_object_unref (self->pipeline);
          pthread_mutex_unlock(self->player_lock);
          return;
      }
    }

    //Set NULL state to clear buffers
    ret = gst_element_set_state (self->pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the ready state.\n");
        gst_object_unref (self->pipeline);
        pthread_mutex_unlock(self->player_lock);
        return;
    }

    //Restore READY state to dispatch state_changed event
    ret = gst_element_set_state (self->pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the ready state.\n");
        gst_object_unref (self->pipeline);
        pthread_mutex_unlock(self->player_lock);
        return;
    } else {
      printf("pipeline state set to GST_STATE_READY.\n");
    }
    pthread_mutex_unlock(self->player_lock);
    // gtk_widget_set_visible(self->canvas, FALSE);
}

void OnvifPlayer__play(OnvifPlayer* self){
  pthread_mutex_lock(self->player_lock);
  printf("OnvifPlayer__play \n");
  GstStateChangeReturn ret;
  ret = gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
      g_printerr ("Unable to set the pipeline to the playing state.\n");
      gst_object_unref (self->pipeline);
      pthread_mutex_unlock(self->player_lock);
      return;
  }
  pthread_mutex_unlock(self->player_lock);
}

GtkWidget * OnvifDevice__createCanvas(OnvifPlayer *self){

  self->canvas = gtk_drawing_area_new ();
  gtk_widget_set_visible(self->canvas, FALSE);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  //TODO Support Gl. Currently not well supported by WSL
  gtk_widget_set_double_buffered (self->canvas, FALSE);
#pragma GCC diagnostic pop
  g_signal_connect (self->canvas, "realize", G_CALLBACK (realize_cb), self);
  g_signal_connect (self->src, "select-stream", G_CALLBACK (find_backchannel),self);
  return self->canvas;
}