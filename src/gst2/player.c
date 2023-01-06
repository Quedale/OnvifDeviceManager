#include "player.h"
#include "overlay.h"
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
  gst_element_set_state (data->backpipe, GST_STATE_READY);
  gst_element_set_state (data->pipeline, GST_STATE_READY);
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  g_print ("End-Of-Stream reached.\n");
  gst_element_set_state (data->backpipe, GST_STATE_READY);
  gst_element_set_state (data->pipeline, GST_STATE_READY);
}


void pix_handler(GstBus * bus, GstMessage * message, OnvifPlayer *player, const GstStructure *s){
  /* only interested in element messages from our gdkpixbufsink */
  if (message->src != GST_OBJECT_CAST (player->sink))
    return;

  const GstStructure * structure = gst_message_get_structure (message);
  const GValue *val = gst_structure_get_value (structure, "pixbuf");
  g_return_if_fail (val != NULL);
  
  GstPad * sinkpad = gst_element_get_static_pad (player->sink, "sink");
  GstCaps * sink_caps = gst_pad_get_current_caps(sinkpad);
  GstStructure *sink_struct = gst_caps_get_structure (sink_caps, 0);

  //Get stream resolution from caps
  gint caps_width;
  gint caps_height;
  if (gst_structure_has_field (sink_struct, "width")) {
    gst_structure_get_int(sink_struct,"width",&caps_width);
  } else {
    GST_ERROR("No width in sink caps.");
    return;
  }
  if (gst_structure_has_field (sink_struct, "height")) {
    gst_structure_get_int(sink_struct,"height",&caps_height);
  } else {
    GST_ERROR("No height in sink caps.");
    return;
  }

  //Clear previous content
  gtk_image_clear(GTK_IMAGE(player->canvas_img));

  //Extract widget size
  GtkAllocation allocation;
  gtk_widget_get_allocation(player->canvas,&allocation);
  
  
  //Extract frame
  GdkPixbuf *pixbuf = GDK_PIXBUF (g_value_dup_object (val));

  //Extract window measurements
  GtkWidget *root = gtk_widget_get_toplevel (GTK_WIDGET (player->canvas));
  GtkAllocation window_allocation;
  gtk_widget_get_allocation(root,&window_allocation);
  
  //Caculate widget size. (WINDOWSIZE - WIDGETPOSITION - MARGINTOEDGE) TODO Find the margin programatically
  int actual_wwidth = window_allocation.width - allocation.x - 10;
  int actual_wheight = window_allocation.height - allocation.y - 10;

  //Minumin size check
  GtkRequisition min;
  gtk_widget_get_preferred_size(gtk_widget_get_parent(gtk_widget_get_parent(player->canvas)), &min, NULL);
  if(actual_wwidth < min.width){
    actual_wwidth = min.width;
  }

  if(actual_wheight < min.height){
    actual_wheight = min.height;
  }

  double set_w;
  double set_h;

  //Scale up and down
  set_h=  caps_height * (double)actual_wwidth / caps_width;
  set_w = caps_width * (double)actual_wheight / caps_height;

  // //Scale down only
  // if(caps_width > actual_wwidth){
  //   set_h=  caps_height * (double)actual_wwidth / caps_width;
  // } else {
  //   set_h = caps_height;
  // }
  // if(caps_height > actual_wheight){
  //   set_w = caps_width * (double)actual_wheight / caps_height;
  // } else {
  //   // set_w = caps_width;
  // }

  if(set_h > actual_wheight){
    set_h = actual_wheight;
  }

  if(set_w > actual_wwidth){
    set_w = actual_wwidth;
  }
  
  //Rescale and keep original pointer for cleanup
  GdkPixbuf * npixbuf = gdk_pixbuf_scale_simple (pixbuf,set_w,set_h,GDK_INTERP_NEAREST); 
  
  //Clean up
  g_object_unref (pixbuf);

  //Update image on screen
  gtk_image_set_from_pixbuf (GTK_IMAGE (player->canvas_img), npixbuf);

  //Clean up
  g_object_unref (npixbuf);
}

gboolean level_handler(GstBus * bus, GstMessage * message, OnvifPlayer *player, const GstStructure *s){

  gint channels;
  GstClockTime endtime;
  gdouble rms_dB;
  gdouble output = 0;
  const GValue *array_val;
  const GValue *value;
  GValueArray *rms_arr;
  gint i;

  if (!gst_structure_get_clock_time (s, "endtime", &endtime))
    g_warning ("Could not parse endtime");

  /* the values are packed into GValueArrays with the value per channel */
  array_val = gst_structure_get_value (s, "rms");
  rms_arr = (GValueArray *) g_value_get_boxed (array_val);

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

    output = output + 100 + rms_dB / 7; 
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

static void 
message_handler (GstBus * bus, GstMessage * message, gpointer p)
{ 
  OnvifPlayer *player = (OnvifPlayer *) p;
  switch(GST_MESSAGE_TYPE(message)){
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
        level_handler(bus,message,player,s);
      } else if (!strcmp (name, "pixbuf") || !strcmp (name, "preroll-pixbuf")) {
        pix_handler(bus,message,player,s);
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
      printf("msg : GST_MESSAGE_ASYNC_DONE\n");
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
    //Doesnt exists in gstreamer 1.16
    // case GST_MESSAGE_DEVICE_CHANGED:
    //  printf("msg : GST_MESSAGE_DEVICE_CHANGED\n");
    //  break;
    // case GST_MESSAGE_INSTANT_RATE_REQUEST:
    //  printf("msg : GST_MESSAGE_INSTANT_RATE_REQUEST\n");
      break;
    case GST_MESSAGE_ANY:
      printf("msg : GST_MESSAGE_ANY\n");
      break;
    default:
      printf("msg : default....");

  }
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

  // g_signal_emit_by_name (player->src, "push-backchannel-buffer", player->back_stream_id, sample, &ret);

  g_signal_emit_by_name (player->src, "push-backchannel-sample", player->back_stream_id, sample, &ret);

  //I know this was added in version 1.21, but it generates //gst_mini_object_unlock: assertion 'state >= SHARE_ONE' failed
  /* Action signal callbacks don't take ownership of the arguments passed, so we must unref the sample here.
   * (The "push-backchannel-buffer" callback unrefs the sample, which is wrong and doesn't work with bindings
   * but could not be changed, hence the new "push-backchannel-sample" callback that behaves correctly.)  */
  // gst_sample_unref (sample);
  
out:
  return ret;
}

void
setup_backchannel_shoveler (OnvifPlayer * player, GstCaps * caps)
{
  GstElement *appsink;
  
  player->backpipe = gst_parse_launch ("autoaudiosrc ! volume name=vol ! "
      "mulawenc ! rtppcmupay ! appsink name=out", NULL);
  if (!player->backpipe)
    g_error ("Could not setup backchannel pipeline");

  appsink = gst_bin_get_by_name (GST_BIN (player->backpipe), "out");
  g_object_set (G_OBJECT (appsink), "caps", caps, "emit-signals", TRUE, NULL);

  player->mic_volume_element = gst_bin_get_by_name (GST_BIN (player->backpipe), "vol");
  g_object_set (G_OBJECT (player->mic_volume_element), "mute", TRUE, NULL);

  g_signal_connect (appsink, "new-sample", G_CALLBACK (new_sample), player);

  g_print ("Playing backchannel shoveler\n");
  gst_element_set_state (player->backpipe, GST_STATE_PLAYING);
}

gboolean OnvifPlayer__is_mic_mute(OnvifPlayer* self) {
  if(GST_IS_ELEMENT(self->mic_volume_element)){
    gboolean v;
    g_object_get (G_OBJECT (self->mic_volume_element), "mute", &v, NULL);
    return v;
  } else {
    return TRUE;
  }
}

void OnvifPlayer__mic_mute(OnvifPlayer* self, gboolean mute) {
  if(GST_IS_ELEMENT(self->mic_volume_element)){
    g_object_set (G_OBJECT (self->mic_volume_element), "mute", mute, NULL);
  }
}

gboolean
find_backchannel (GstElement * rtspsrc, guint idx, GstCaps * caps,
    OnvifPlayer *player G_GNUC_UNUSED)
{
  GstStructure *s;
  gchar *caps_str = gst_caps_to_string (caps);
  g_free (caps_str);
  printf("OnvifDevice__find_backchannel\n");
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


    //TODO Check caps before linking (audio to vidoe and video to audio invalid linking)
    /* We can now link this pad with the rtsp-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    ret = gst_pad_link (pad, sinkpad);

    if (GST_PAD_LINK_FAILED (ret)) {
      g_print("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(decoder));
    }

    gst_object_unref (sinkpad);
}

void create_pipeline(OnvifPlayer *self){
  GstElement *vdecoder;
  GstElement *videoqueue0;
  GstElement *videoconvert;
  GstElement *overlay_comp;
  GstElement *adecoder;
  GstElement *audioqueue0;
  GstElement *audioconvert;
  GstElement *level;
  GstElement *audio_sink;

  /* Create the elements */
  self->src = gst_element_factory_make ("rtspsrc", "rtspsrc");
  //Video pipe
  vdecoder = gst_element_factory_make ("decodebin", "videodecodebin");
  videoqueue0 = gst_element_factory_make ("queue", "videoqueue0");
  videoconvert = gst_element_factory_make ("videoconvert", "videoconvert0");
  overlay_comp = gst_element_factory_make ("overlaycomposition", NULL);
  self->sink = gst_element_factory_make ("gdkpixbufsink", "vsink");
  // g_object_set (G_OBJECT (self->sink), "sync", FALSE, NULL);
  g_object_set (G_OBJECT (self->src), "latency", 0, NULL);
  g_object_set (G_OBJECT (self->src), "teardown-timeout", 1, NULL); 
  g_object_set (G_OBJECT (self->src), "backchannel", 1, NULL);
  g_object_set (G_OBJECT (self->src), "user-agent", "OnvifDeviceManager-Linux-0.0", NULL);
  g_object_set (G_OBJECT (self->src), "do-retransmission", TRUE, NULL);
  g_object_set (G_OBJECT (self->src), "onvif-mode", TRUE, NULL);
  g_object_set (self->sink, "qos", FALSE, "max-lateness", (gint64) - 1, NULL);

  //Audio pipe
  adecoder = gst_element_factory_make ("decodebin", "audiodecodebin");
  audioqueue0 = gst_element_factory_make ("queue", "audioqueue0");
  audioconvert = gst_element_factory_make ("audioconvert", "audioconvert0");
  level = gst_element_factory_make("level","level0");
  audio_sink = gst_element_factory_make ("autoaudiosink", "autoaudiosink0");
  // g_object_set (G_OBJECT (audio_sink), "sync", FALSE, NULL);
  g_object_set (G_OBJECT (level), "post-messages", TRUE, NULL);
  //For smoother responsiveness and more accurate value, lowered interval by 2x
  g_object_set (G_OBJECT (level), "interval", 50000000, NULL);

  /* Create the empty pipeline */
  self->pipeline = malloc(sizeof(GstElement *));
  self->pipeline = gst_pipeline_new ("onvif-pipeline");

  //Make sure: Every elements was created ok
  if (!self->pipeline || \
      !self->src || \
      !vdecoder || \
      !videoqueue0 || \
      !videoconvert || \
      !overlay_comp || \
      !self->sink || \
      !adecoder || \
      !audioqueue0 || \
      !audioconvert || \
      !level || \
      !audio_sink) {
      g_printerr ("One of the elements wasn't created... Exiting\n");
      return;
  }

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->pipeline), \
    self->src, \
    vdecoder, \
    videoqueue0, \
    videoconvert, \
    overlay_comp, \
    self->sink, \
    adecoder, \
    audioqueue0, \
    audioconvert, \
    level, \
    audio_sink, NULL);

  // Link confirmation
  if (!gst_element_link_many (videoqueue0, \
    videoconvert, \
    overlay_comp, \
    self->sink, NULL)){
      g_warning ("Linking part (A)-2 Fail...");
      return;
  }

  // Link confirmation
  if (!gst_element_link_many (audioqueue0, audioconvert, level, audio_sink, NULL)){
      g_warning ("Linking part (B)-2 Fail...");
      return;
  }

  // Dynamic Pad Creation
  if(! g_signal_connect (self->src, "pad-added", G_CALLBACK (on_pad_added),vdecoder))
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
   
  if(! g_signal_connect (overlay_comp, "draw", G_CALLBACK (draw_overlay), self)){
    g_warning ("overlay draw callback Fail...");
  }

  if(! g_signal_connect (overlay_comp, "caps-changed",G_CALLBACK (prepare_overlay), self->overlay_state)){
    g_warning ("overlay draw callback Fail...");
  }

  /* set up bus */
  GstBus *bus = gst_element_get_bus (self->pipeline);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", G_CALLBACK (message_handler), self);
  gst_object_unref (bus);
}

void OnvifPlayer__init(OnvifPlayer* self) {

    self->level = 0;
    self->onvifDeviceList = OnvifDeviceList__create();
    self->device = NULL;
    self->mic_volume_element = NULL;
    self->state = GST_STATE_NULL;
    self->player_lock =malloc(sizeof(pthread_mutex_t));

    self->overlay_state = malloc(sizeof(OverlayState));
    self->overlay_state->valid = 0;
    
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
    free(self->overlay_state);
    free(self->player_lock);
    OnvifDeviceList__destroy(self->onvifDeviceList);
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

  /*
    Creating a GtkImage and GtkEventBox which will later be used
      in the gdkpixbuffsink message handler to display the content

      TODO : Test GtkDrawingArea as better alternative
  */ 
  self->canvas = gtk_event_box_new ();
  gtk_widget_set_visible(self->canvas, FALSE);
  self->canvas_img = gtk_image_new();
  gtk_container_add (GTK_CONTAINER (self->canvas), self->canvas_img);
  g_signal_connect (self->src, "select-stream", G_CALLBACK (find_backchannel),self);
  return self->canvas;
}