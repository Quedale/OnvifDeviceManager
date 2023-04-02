#include "player.h"
#include "overlay.h"
#include <unistd.h>
#include "gtk/gstgtkbasesink.h"
#include <math.h>
#include <unistd.h>

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  GError *err;
  gchar *debug_info;

  /* Print error details on the screen */
  gst_message_parse_error (msg, &err, &debug_info);
  GST_ERROR ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  GST_ERROR ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);


  if(GST_IS_ELEMENT(msg->src)){
    if(GST_IS_ELEMENT(data->backpipe)){
      gst_element_set_state (data->backpipe, GST_STATE_NULL);
    }
    gst_element_set_state (data->pipeline, GST_STATE_NULL);
  }

  if(data->retry < 3){
    data->retry++;
    printf("****************************************************\n");
    printf("****************************************************\n");
    printf("Retry attempt #%i\n",data->retry);
    printf("****************************************************\n");
    printf("****************************************************\n");
    sleep(1);
    OnvifPlayer__play(data);
  } else {
    //Hide loading on error
    gtk_widget_hide(data->loading_handle);
  }
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  GST_ERROR ("End-Of-Stream reached.\n");
  if(GST_IS_ELEMENT(data->backpipe)){
    gst_element_set_state (data->backpipe, GST_STATE_NULL);
  }
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
}

void level_handler(GstBus * bus, GstMessage * message, OnvifPlayer *player, const GstStructure *s){

  gint channels;
  GstClockTime endtime;
  gdouble peak_dB;
  gdouble peak;
  const gdouble max_variance = 15;
  const GValue *list;
  const GValue *value;
  GValueArray *peak_arr;
  gdouble output = 0;
  gint i;

  if (!gst_structure_get_clock_time (s, "endtime", &endtime))
    g_warning ("Could not parse endtime");

  list = gst_structure_get_value (s, "peak");
  peak_arr = (GValueArray *) g_value_get_boxed (list);
  channels = peak_arr->n_values;

  for (i = 0; i < channels; ++i) {
    // FIXME 'g_value_array_get_nth' is deprecated: Use 'GArray' instead
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    value = g_value_array_get_nth (peak_arr, i);
    peak_dB = g_value_get_double (value);
    #pragma GCC diagnostic pop

    /* converting from dB to normal gives us a value between 0.0 and 1.0 */
    peak = pow (10, peak_dB / 20);

    //Add up all channels peaks
    output = output + peak * 100; 
  }

  //Optionally only get the highest peak instead of calculating average.
  //Average output of all channels
  output = output / channels;

  //Set Output value on player
  if(output == player->level){
    //Ignore
  } else if( output > player->level){
    //Set new peak
    player->level = output;
  } else {
    //Lower to a maximum drop of 15 for a graphical decay
    gdouble variance = player->level-output;
    if(variance > max_variance){
      player->level=player->level-max_variance; //The decay value has to match the interval set. We are dealing with 2x faster interval therefor 15/2=7.5
    } else {
      player->level = output;
    }
  }

}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void
state_changed_cb (GstBus * bus, GstMessage * msg, OnvifPlayer * data)
{
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  /* Using video_bin for state change since the pipeline is PLAYING before the videosink */
  if (data->video_bin != NULL 
        && GST_IS_OBJECT(data->video_bin) 
        &&  GST_MESSAGE_SRC (msg) == GST_OBJECT (data->video_bin)) {
    data->state = new_state;
    if(data->state == GST_STATE_PLAYING){
      gtk_widget_set_visible(data->canvas, TRUE);
      //Hide loading after stream successfully started
      gtk_widget_hide(data->loading_handle);
    } else {
      gtk_widget_set_visible(data->canvas, FALSE);
    }
    printf ("State set to %s for %s\n", gst_element_state_get_name (new_state), GST_OBJECT_NAME (msg->src));
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
      eos_cb(bus,message,player);
      break;
    case GST_MESSAGE_ERROR:
      error_cb(bus,message,player);
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

#if (GST_PLUGINS_BASE_VERSION_MAJOR >= 1) && (GST_PLUGINS_BASE_VERSION_MINOR >= 21) //GST_PLUGINS_BASE_VERSION_MICRO
  g_signal_emit_by_name (player->src, "push-backchannel-sample", player->back_stream_id, sample, &ret);
#else
  g_signal_emit_by_name (player->src, "push-backchannel-buffer", player->back_stream_id, sample, &ret);
#endif
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
  printf("OnvifPlayer__find_backchannel\n");
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
static void on_pad_added (GstElement *element, GstPad *new_pad, gpointer data){
    GstPad *sinkpad;
    GstPadLinkReturn ret;
    GstElement *decoder = (GstElement *) data;

    // GstCaps * pad_caps = gst_pad_get_current_caps(new_pad);
    // gchar * caps_str = gst_caps_serialize(pad_caps,0);
    // printf("on_pad_added caps_str : %s\n",caps_str);

    /* We can now link this pad with the rtsp-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    ret = gst_pad_link (new_pad, sinkpad);

    if (GST_PAD_LINK_FAILED (ret)) {
      g_print("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(decoder));
    }

    gst_object_unref (sinkpad);
}

static GstElement * create_audio_bin(OnvifPlayer * self){
  GstPad *pad, *ghostpad;
  GstElement *aqueue, *decoder, *convert, *level, *sink;

  self->audio_bin = gst_bin_new("audiobin");
  aqueue = gst_element_factory_make ("queue", NULL);
  decoder = gst_element_factory_make ("decodebin", NULL);
  convert = gst_element_factory_make ("audioconvert", NULL);
  level = gst_element_factory_make("level",NULL);
  sink = gst_element_factory_make ("autoaudiosink", NULL);

  if (!self->audio_bin ||
      !aqueue ||
      !decoder ||
      !convert ||
      !level ||
      !sink) {
      GST_ERROR ("One of the video elements wasn't created... Exiting\n");
      return NULL;
  }

  g_object_set (G_OBJECT (aqueue), "flush-on-eos", TRUE, NULL);
  g_object_set (G_OBJECT (aqueue), "max-size-buffers", 1, NULL);

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->audio_bin),
    aqueue,
    decoder,
    convert,
    level,
    sink, NULL);

  // Link confirmation
  if (!gst_element_link_many (aqueue,
    decoder, NULL)){
      g_warning ("Linking audio part (A)-1 Fail...");
      return NULL;
  }

  // Link confirmation
  if (!gst_element_link_many (convert,
    level,
    sink, NULL)){
      g_warning ("Linking audio part (A)-2 Fail...");
      return NULL;
  }

  // Dynamic Pad Creation
  if(! g_signal_connect (decoder, "pad-added", G_CALLBACK (on_pad_added),convert))
  {
      g_warning ("Linking (A)-1 part (2) with part (A)-2 Fail...");
  }

  pad = gst_element_get_static_pad (aqueue, "sink");
  if (!pad) {
      // TODO gst_object_unref 
      GST_ERROR("unable to get decoder static sink pad");
      return NULL;
  }

  ghostpad = gst_ghost_pad_new ("bin_sink", pad);
  gst_element_add_pad (self->audio_bin, ghostpad);
  gst_object_unref (pad);

  return self->audio_bin;
}

static GstElement * create_video_bin(OnvifPlayer * self){
  GstElement *vqueue, *vdecoder, *videoconvert, *overlay_comp;
  GstPad *pad, *ghostpad;

  self->video_bin = gst_bin_new("video_bin");
  vqueue = gst_element_factory_make ("queue", NULL);
  vdecoder = gst_element_factory_make ("decodebin", NULL);
  videoconvert = gst_element_factory_make ("videoconvert", NULL);
  overlay_comp = gst_element_factory_make ("overlaycomposition", NULL);
  self->sink = gst_element_factory_make ("gtkcustomsink", NULL);

  if (!self->video_bin ||
      // !capsfilter ||
      !vqueue ||
      !vdecoder ||
      !videoconvert ||
      !overlay_comp ||
      !self->sink) {
      GST_ERROR ("One of the video elements wasn't created... Exiting\n");
      return NULL;
  }


  g_object_set (G_OBJECT (vqueue), "flush-on-eos", TRUE, NULL);
  g_object_set (G_OBJECT (vqueue), "max-size-buffers", 1, NULL);

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->video_bin),
    vqueue,
    vdecoder,
    videoconvert,
    overlay_comp,
    self->sink, NULL);

  // Link confirmation
  if (!gst_element_link_many (vqueue,
    vdecoder, NULL)){
      g_warning ("Linking video part (A)-1 Fail...");
      return NULL;
  }

  // Link confirmation
  if (!gst_element_link_many (videoconvert,
    overlay_comp,
    self->sink, NULL)){
      g_warning ("Linking video part (A)-2 Fail...");
      return NULL;
  }

  // Dynamic Pad Creation
  if(! g_signal_connect (vdecoder, "pad-added", G_CALLBACK (on_pad_added),videoconvert))
  {
      g_warning ("Linking (A)-1 part with part (A)-2 Fail...");
  }

  pad = gst_element_get_static_pad (vqueue, "sink");
  if (!pad) {
      // TODO gst_object_unref 
      GST_ERROR("unable to get decoder static sink pad");
      return NULL;
  }

  if(! g_signal_connect (overlay_comp, "draw", G_CALLBACK (draw_overlay), self)){
    GST_ERROR ("overlay draw callback Fail...");
  }

  if(! g_signal_connect (overlay_comp, "caps-changed",G_CALLBACK (prepare_overlay), self->overlay_state)){
    GST_ERROR ("overlay caps-changed callback Fail...");
    return NULL;
  }

  ghostpad = gst_ghost_pad_new ("bin_sink", pad);
  gst_element_add_pad (self->video_bin, ghostpad);
  gst_object_unref (pad);

  if(!self->canvas){
    self->canvas = gst_gtk_base_custom_sink_acquire_widget(GST_GTK_BASE_CUSTOM_SINK(self->sink));
  } else {
    gst_gtk_base_custom_sink_set_widget(GST_GTK_BASE_CUSTOM_SINK(self->sink),GTK_GST_BASE_CUSTOM_WIDGET(self->canvas));
  }
  gst_gtk_base_custom_sink_set_parent(GST_GTK_BASE_CUSTOM_SINK(self->sink),self->canvas_handle);

  return self->video_bin;
}

/* Dynamically link */
static void on_rtsp_pad_added (GstElement *element, GstPad *new_pad, OnvifPlayer * data){
  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (element));

  GstPadLinkReturn pad_ret;
  GstPad *sink_pad = NULL;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  

  gint payload_v;
  gst_structure_get_int(new_pad_struct,"payload", &payload_v);

  // gchar * caps_str = gst_caps_serialize(new_pad_caps,0);
  // gchar * new_pad_type = gst_structure_get_name (new_pad_struct);
  // printf("caps_str : %s\n",caps_str);

  if (payload_v == 96) {
    gst_event_new_flush_stop(0);
    GstElement * video_bin = create_video_bin(data);

    gst_bin_add_many (GST_BIN (data->pipeline), video_bin, NULL);

    sink_pad = gst_element_get_static_pad (video_bin, "bin_sink");

    pad_ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (pad_ret)) {
      g_printerr ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(video_bin));
      goto exit;
    }

    gst_element_sync_state_with_parent(video_bin);
  } else if (payload_v == 0) {
    const gchar * audio_format_v = gst_structure_get_string(new_pad_struct,"encoding-name");
    if(!strcmp(audio_format_v,"PCMU")){
      GstElement * audio_bin = create_audio_bin(data);

      gst_bin_add_many (GST_BIN (data->pipeline), audio_bin, NULL);

      sink_pad = gst_element_get_static_pad (audio_bin, "bin_sink");
      pad_ret = gst_pad_link (new_pad, sink_pad);
      if (GST_PAD_LINK_FAILED (pad_ret)) {
        g_printerr ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(audio_bin));
        goto exit;
      }

      gst_element_sync_state_with_parent(audio_bin);
    } else {
      GST_FIXME("Add support to other audio format...\n");
    }
  } else {
    GST_FIXME("Support other payload formats\n");
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);
  if (sink_pad != NULL)
    gst_object_unref (sink_pad);
}

void create_pipeline(OnvifPlayer *self){

  /* Create the empty pipeline */
  self->pipeline = gst_pipeline_new ("onvif-pipeline");

  /* Create the elements */
  self->src = gst_element_factory_make ("rtspsrc", "rtspsrc");

  if (!self->pipeline
    || !self->src){
      GST_ERROR ("One of the elements wasn't created... Exiting\n");
      return;
  }

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->pipeline), self->src, NULL);

  // Dynamic Pad Creation
  if(! g_signal_connect (self->src, "pad-added", G_CALLBACK (on_rtsp_pad_added),self))
  {
      g_warning ("Linking part (1) with part (A)-1 Fail...");
  }

  if(!g_signal_connect (self->src, "select-stream", G_CALLBACK (find_backchannel),self))
  {
      GST_WARNING ("Fail to connect select-stream signal...");
  }

  g_object_set (G_OBJECT (self->src), "buffer-mode", 1, NULL);
  g_object_set (G_OBJECT (self->src), "latency", 0, NULL);
  g_object_set (G_OBJECT (self->src), "teardown-timeout", 0, NULL); 
  g_object_set (G_OBJECT (self->src), "backchannel", 1, NULL);
  g_object_set (G_OBJECT (self->src), "user-agent", "OnvifDeviceManager-Linux-0.0", NULL);
  g_object_set (G_OBJECT (self->src), "do-retransmission", FALSE, NULL);
  g_object_set (G_OBJECT (self->src), "onvif-mode", TRUE, NULL);
  g_object_set (G_OBJECT (self->src), "is-live", TRUE, NULL);

  /* set up bus */
  GstBus *bus = gst_element_get_bus (self->pipeline);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", G_CALLBACK (message_handler), self);
  gst_object_unref (bus);
}

void OnvifPlayer__init(OnvifPlayer* self) {

    self->level = 0;
    self->retry = 0;
    self->device_list = DeviceList__create();
    self->device = NULL;
    self->mic_volume_element = NULL;
    self->state = GST_STATE_NULL;
    self->player_lock =malloc(sizeof(pthread_mutex_t));

    self->overlay_state = malloc(sizeof(OverlayState));
    self->overlay_state->valid = 0;
    self->canvas_handle = NULL;
    self->canvas = NULL;
    self->video_bin = NULL;
    self->audio_bin = NULL;

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
    DeviceList__destroy(self->device_list);
}

void OnvifPlayer__destroy(OnvifPlayer* self) {
  if (self) {
     OnvifPlayer__reset(self);
     free(self);
  }
}

void OnvifPlayer__set_playback_url(OnvifPlayer* self, char *url) {
    pthread_mutex_lock(self->player_lock);
    printf("set location : %s\n",url);
    g_object_set (G_OBJECT (self->src), "location", url, NULL);
    pthread_mutex_unlock(self->player_lock);
}

void OnvifPlayer__stop(OnvifPlayer* self){
    pthread_mutex_lock(self->player_lock);
    printf("OnvifPlayer__stop \n");
    self->retry = 0;
    GstStateChangeReturn ret;

    //Backchannel clean up
    if(GST_IS_ELEMENT(self->backpipe)){
      ret = gst_element_set_state (self->backpipe, GST_STATE_NULL);
      gst_object_unref (self->backpipe);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          GST_ERROR ("Unable to set the backpipe to the ready state.\n");
          goto stop_out;
      }
    }

    //Set NULL state to clear buffers
    if(GST_IS_ELEMENT(self->pipeline)){
      ret = gst_element_set_state (self->pipeline, GST_STATE_NULL);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          GST_ERROR ("Unable to set the pipeline to the ready state.\n");
          goto stop_out;
      }
    }
stop_out: 

  //Destroy old pipeline
  gst_object_unref (self->pipeline);
  if(GST_IS_ELEMENT(self->backpipe)){
    gst_object_unref (self->backpipe);
  }

  // Create new pipeline
  create_pipeline(self);

  pthread_mutex_unlock(self->player_lock);
}

void OnvifPlayer__play(OnvifPlayer* self){
  pthread_mutex_lock(self->player_lock);
  printf("OnvifPlayer__play \n");

  //Display loading
  gtk_widget_show(self->loading_handle);

  GstStateChangeReturn ret;
  ret = gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
      GST_ERROR ("Unable to set the pipeline to the playing state.\n");
      gst_object_unref (self->pipeline);
      pthread_mutex_unlock(self->player_lock);
      return;
  }
  pthread_mutex_unlock(self->player_lock);
}

GtkWidget * OnvifDevice__createCanvas(OnvifPlayer *self){

  self->canvas_handle = gtk_grid_new ();
  return self->canvas_handle;
}