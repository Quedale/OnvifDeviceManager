#include "player.h"
#include "gtk/gstgtkbasesink.h"
#include "clogger.h"
#include "url_parser.h"

typedef struct _RtspPlayer {
  GstElement *pipeline;
  GstElement *src;  /* RtspSrc to support backchannel */
  GstElement *sink;  /* Video Sink */
  int pad_found;
  GstElement * video_bin;
  int no_video;
  int video_done;
  GstElement * audio_bin;
  int no_audio;
  int audio_done;
  
  //Backpipe related properties
  int enable_backchannel;
  RtspBackchannel * backchannel;
  GstVideoOverlay *overlay; //Overlay rendered on the canvas widget
  OverlayState *overlay_state;

  //Keep location to used on retry
  char * location;
  //Retry count
  int retry;
  //Playing or trying to play
  int playing;

  void (*retry_callback)(RtspPlayer *, void * user_data);
  void * retry_user_data;

  void (*error_callback)(RtspPlayer *, void * user_data);
  void * error_user_data;

  void (*stopped_callback)(RtspPlayer *, void * user_data);
  void * stopped_user_data;

  void (*start_callback)(RtspPlayer *, void * user_data);
  void * start_user_data;

  //Grid holding the canvas
  GtkWidget *canvas_handle;
  //Canvas used to draw stream
  GtkWidget *canvas;
  //Set if the drawing area should over stretch
  int allow_overscale;
  
  P_MUTEX_TYPE prop_lock;
  P_MUTEX_TYPE player_lock;

  char * user;
  char * pass;

  char * port_fallback;
} RtspPlayer;

void _RtspPlayer__stop(RtspPlayer* self, int notify);

static void warning_cb (GstBus *bus, GstMessage *msg, RtspPlayer *data) {
  GError *err;
  gchar *debug_info;
  gst_message_parse_warning (msg, &err, &debug_info);
  C_WARN ("Warning received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
  C_WARN ("Debugging information: %s", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);
}

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, RtspPlayer *data) {
  GError *err;
  gchar *debug_info;
  
  gst_message_parse_error (msg, &err, &debug_info);
  P_MUTEX_LOCK(data->player_lock);
  if(err->code == GST_RESOURCE_ERROR_SETTINGS && data->enable_backchannel){
    C_WARN ("Backchannel unsupported. Downgrading...");
    data->enable_backchannel = 0;
    data->retry--; //This doesn't count as a try. Finding out device capabilities count has handshake
  } else if(err->code == GST_RESOURCE_ERROR_NOT_AUTHORIZED ||
      err->code == GST_RESOURCE_ERROR_NOT_FOUND){
    C_ERROR("Non-recoverable error encountered.");
    data->playing = 0;
  } else if((err->code == GST_RESOURCE_ERROR_OPEN_READ ||
            err->code == GST_RESOURCE_ERROR_OPEN_WRITE ||
            err->code == GST_RESOURCE_ERROR_OPEN_READ_WRITE) &&
            data->port_fallback){
    char * oldloc = data->location;
    data->location = URL__set_port(oldloc, data->port_fallback);
    C_INFO("Stream Correction : '%s' --> '%s'\n",oldloc, data->location);
    free(oldloc);

  } else if(err->code == GST_RESOURCE_ERROR_READ){
    data->retry--; //This doesn't count as a try (After this, will generate GST_RESOURCE_ERROR_OPEN_)
  }
  P_MUTEX_UNLOCK(data->player_lock);

  C_ERROR ("Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
  C_ERROR ("Debugging information: %s", debug_info ? debug_info : "none");
  C_ERROR ("Error code : %d",err->code);

  g_clear_error (&err);
  g_free (debug_info);

  C_DEBUG("retry[%d] playing[%d]", data->retry, data->playing);
  if(data->retry < 3 && data->playing == 1 && data->retry_callback){
    //Stopping player after if condition because "playing" gets reset
    _RtspPlayer__stop(data,0);
    P_MUTEX_LOCK(data->player_lock);
    data->playing = 1; //Restore playing flag after calling stop to allow retry
    data->retry++; 
    P_MUTEX_UNLOCK(data->player_lock);
    C_WARN("****************************************************");
    C_WARN("Retry attempt #%i",data->retry);
    C_WARN("****************************************************");
    //Retry signal
    (*(data->retry_callback))(data, data->retry_user_data);
  } else {
    RtspPlayer__stop(data);
    //Error signal
    if(data->error_callback){
      (*(data->error_callback))(data, data->error_user_data);
    }
  }
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, RtspPlayer *data) {
  C_ERROR ("End-Of-Stream reached.\n");
  RtspPlayer__stop(data);
}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void
state_changed_cb (GstBus * bus, GstMessage * msg, RtspPlayer * data)
{
  GstState old_state, new_state, pending_state;

  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  /* Using video_bin for state change since the pipeline is PLAYING before the videosink */
  
  //Check if video exists and pad is linked
  if(data && (data->no_video ||
        (data->video_bin != NULL
        && GST_IS_OBJECT(data->video_bin) 
        &&  GST_MESSAGE_SRC (msg) == GST_OBJECT (data->video_bin)))){
      if(new_state == GST_STATE_PLAYING) {
        data->video_done=1;
      } else {
        if(GTK_IS_WIDGET (data->canvas))
          gtk_widget_set_visible(data->canvas, FALSE);
      }
  }

  //Check if audio exists and pad is linked
  if(data && (data->no_audio ||
        (data->audio_bin != NULL 
        && GST_IS_OBJECT(data->audio_bin) 
        &&  GST_MESSAGE_SRC (msg) == GST_OBJECT (data->audio_bin))) &&
      new_state == GST_STATE_PLAYING) {
    data->audio_done=1;
  }

  //Check that all streams are ready before hiding loading
  if(data && data->video_done && data->audio_done && data->pad_found == 1 &&
      data->pipeline != NULL && GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline)){
    if(GTK_IS_WIDGET (data->canvas))
      gtk_widget_set_visible(data->canvas, TRUE);
    //Stopped changing state signal
    if(data->stopped_callback){
      (*(data->stopped_callback))(data, data->stopped_user_data);
    }
  }

  if(data &&
      ((data->video_bin && GST_MESSAGE_SRC (msg) == (GstObject*) data->video_bin) ||
      (data->audio_bin && GST_MESSAGE_SRC (msg) == (GstObject*) data->audio_bin) ||
      (data->pipeline && GST_MESSAGE_SRC (msg) == (GstObject*) data->pipeline))){
    C_TRACE ("State set to %s for %s\n", gst_element_state_get_name (new_state), GST_OBJECT_NAME (msg->src));
    // printf("[no_audio:%i][no_video:%i][audio_done:%i][video_done:%i][pad_found:%i][pipe:%i]\n",data->no_audio,data->no_video,data->audio_done,data->video_done,data->pad_found,GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline));
  }
}

static void 
message_handler (GstBus * bus, GstMessage * message, gpointer p)
{ 
  const GstStructure *s;
  const gchar *name;
  RtspPlayer *player = (RtspPlayer *) p;
  switch(GST_MESSAGE_TYPE(message)){
    case GST_MESSAGE_UNKNOWN:
      C_DEBUG("msg : GST_MESSAGE_UNKNOWN\n");
      break;
    case GST_MESSAGE_EOS:
      eos_cb(bus,message,player);
      break;
    case GST_MESSAGE_ERROR:
      error_cb(bus,message,player);
      break;
    case GST_MESSAGE_WARNING:
      warning_cb(bus,message,player);
      break;
    case GST_MESSAGE_INFO:
      C_DEBUG("msg : GST_MESSAGE_INFO\n");
      break;
    case GST_MESSAGE_TAG:
      // tag_cb(bus,message,player);
      break;
    case GST_MESSAGE_BUFFERING:
      C_DEBUG("msg : GST_MESSAGE_BUFFERING\n");
      break;
    case GST_MESSAGE_STATE_CHANGED:
      state_changed_cb(bus,message,player);
      break;
    case GST_MESSAGE_STATE_DIRTY:
      C_DEBUG("msg : GST_MESSAGE_STATE_DIRTY\n");
      break;
    case GST_MESSAGE_STEP_DONE:
      C_DEBUG("msg : GST_MESSAGE_STEP_DONE\n");
      break;
    case GST_MESSAGE_CLOCK_PROVIDE:
      C_DEBUG("msg : GST_MESSAGE_CLOCK_PROVIDE\n");
      break;
    case GST_MESSAGE_CLOCK_LOST:
      C_DEBUG("msg : GST_MESSAGE_CLOCK_LOST\n");
      break;
    case GST_MESSAGE_NEW_CLOCK:
      break;
    case GST_MESSAGE_STRUCTURE_CHANGE:
      C_DEBUG("msg : GST_MESSAGE_STRUCTURE_CHANGE\n");
      break;
    case GST_MESSAGE_STREAM_STATUS:
      break;
    case GST_MESSAGE_APPLICATION:
      C_DEBUG("msg : GST_MESSAGE_APPLICATION\n");
      break;
    case GST_MESSAGE_ELEMENT:
      s = gst_message_get_structure (message);
      name = gst_structure_get_name (s);
      if (strcmp (name, "level") == 0) {
        OverlayState__level_handler(bus,message,player->overlay_state,s);
      } else if (strcmp (name, "GstNavigationMessage") == 0 || 
                strcmp (name, "application/x-rtp-source-sdes") == 0){
        //Ignore intentionally left unhandled for now
      } else {
        C_ERROR("Unhandled element msg name : %s//%d\n",name,message->type);
      }
      break;
    case GST_MESSAGE_SEGMENT_START:
      C_DEBUG("msg : GST_MESSAGE_SEGMENT_START\n");
      break;
    case GST_MESSAGE_SEGMENT_DONE:
      C_DEBUG("msg : GST_MESSAGE_SEGMENT_DONE\n");
      break;
    case GST_MESSAGE_DURATION_CHANGED:
      C_DEBUG("msg : GST_MESSAGE_DURATION_CHANGED\n");
      break;
    case GST_MESSAGE_LATENCY:
      break;
    case GST_MESSAGE_ASYNC_START:
      C_DEBUG("msg : GST_MESSAGE_ASYNC_START\n");
      break;
    case GST_MESSAGE_ASYNC_DONE:
      // printf("msg : GST_MESSAGE_ASYNC_DONE\n");
      break;
    case GST_MESSAGE_REQUEST_STATE:
      C_DEBUG("msg : GST_MESSAGE_REQUEST_STATE\n");
      break;
    case GST_MESSAGE_STEP_START:
      C_DEBUG("msg : GST_MESSAGE_STEP_START\n");
      break;
    case GST_MESSAGE_QOS:
      break;
    case GST_MESSAGE_PROGRESS:
      break;
    case GST_MESSAGE_TOC:
      C_DEBUG("msg : GST_MESSAGE_TOC\n");
      break;
    case GST_MESSAGE_RESET_TIME:
      C_DEBUG("msg : GST_MESSAGE_RESET_TIME\n");
      break;
    case GST_MESSAGE_STREAM_START:
      break;
    case GST_MESSAGE_NEED_CONTEXT:
      break;
    case GST_MESSAGE_HAVE_CONTEXT:
      break;
    case GST_MESSAGE_EXTENDED:
      C_DEBUG("msg : GST_MESSAGE_EXTENDED\n");
      break;
    case GST_MESSAGE_DEVICE_ADDED:
      C_DEBUG("msg : GST_MESSAGE_DEVICE_ADDED\n");
      break;
    case GST_MESSAGE_DEVICE_REMOVED:
      C_DEBUG("msg : GST_MESSAGE_DEVICE_REMOVED\n");
      break;
    case GST_MESSAGE_PROPERTY_NOTIFY:
      C_DEBUG("msg : GST_MESSAGE_PROPERTY_NOTIFY\n");
      break;
    case GST_MESSAGE_STREAM_COLLECTION:
      C_DEBUG("msg : GST_MESSAGE_STREAM_COLLECTION\n");
      break;
    case GST_MESSAGE_STREAMS_SELECTED:
      C_DEBUG("msg : GST_MESSAGE_STREAMS_SELECTED\n");
      break;
    case GST_MESSAGE_REDIRECT:
      C_DEBUG("msg : GST_MESSAGE_REDIRECT\n");
      break;
    //Doesnt exists in gstreamer 1.16
    // case GST_MESSAGE_DEVICE_CHANGED:
    //  printf("msg : GST_MESSAGE_DEVICE_CHANGED\n");
    //  break;
    // case GST_MESSAGE_INSTANT_RATE_REQUEST:
    //  printf("msg : GST_MESSAGE_INSTANT_RATE_REQUEST\n");
      break;
    case GST_MESSAGE_ANY:
      C_DEBUG("msg : GST_MESSAGE_ANY\n");
      break;
    default:
      C_DEBUG("msg : default....");

  }
}

gboolean RtspPlayer__is_mic_mute(RtspPlayer* self) {
  return RtspBackchannel__is_mute(self->backchannel);
}

void RtspPlayer__mic_mute(RtspPlayer* self, gboolean mute) {
  RtspBackchannel__mute(self->backchannel, mute);
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

static GstElement * create_audio_bin(RtspPlayer * self){
  GstPad *pad, *ghostpad;
  GstElement *decoder, *convert, *level, *sink;

  self->audio_bin = gst_bin_new("audiobin");
  decoder = gst_element_factory_make ("decodebin3", NULL);
  convert = gst_element_factory_make ("audioconvert", NULL);
  level = gst_element_factory_make("level",NULL);
  sink = gst_element_factory_make ("autoaudiosink", NULL);
  g_object_set (G_OBJECT (sink), "sync", FALSE, NULL);
  if (!self->audio_bin ||
      !decoder ||
      !convert ||
      !level ||
      !sink) {
      C_ERROR ("One of the video elements wasn't created... Exiting\n");
      return NULL;
  }

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->audio_bin),
    decoder,
    convert,
    level,
    sink, NULL);


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

  pad = gst_element_get_static_pad (decoder, "sink");
  if (!pad) {
      // TODO gst_object_unref 
      C_ERROR("unable to get decoder static sink pad");
      return NULL;
  }

  ghostpad = gst_ghost_pad_new ("bin_sink", pad);
  gst_element_add_pad (self->audio_bin, ghostpad);
  gst_object_unref (pad);

  return self->audio_bin;
}

static GstElement * create_video_bin(RtspPlayer * self){
  GstElement *vdecoder, *videoconvert, *overlay_comp;
  GstPad *pad, *ghostpad;

  self->video_bin = gst_bin_new("video_bin");
  vdecoder = gst_element_factory_make ("decodebin3", NULL);
  videoconvert = gst_element_factory_make ("videoconvert", NULL);
  overlay_comp = gst_element_factory_make ("overlaycomposition", NULL);
  self->sink = gst_element_factory_make ("gtkcustomsink", NULL);
  gst_base_sink_set_qos_enabled(GST_BASE_SINK_CAST(self->sink),FALSE);
  gst_base_sink_set_sync(GST_BASE_SINK_CAST(self->sink),FALSE);
  gst_gtk_base_custom_sink_set_expand(GST_GTK_BASE_CUSTOM_SINK(self->sink),self->allow_overscale);

  if (!self->video_bin ||
      !vdecoder ||
      !videoconvert ||
      !overlay_comp ||
      !self->sink) {
      C_ERROR ("One of the video elements wasn't created... Exiting\n");
      return NULL;
  }

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->video_bin),
    vdecoder,
    videoconvert,
    overlay_comp,
    self->sink, NULL);

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

  pad = gst_element_get_static_pad (vdecoder, "sink");
  if (!pad) {
      // TODO gst_object_unref 
      C_ERROR("unable to get decoder static sink pad");
      return NULL;
  }

  if(! g_signal_connect (overlay_comp, "draw", G_CALLBACK (OverlayState__draw_overlay), self->overlay_state)){
    C_ERROR ("overlay draw callback Fail...");
  }

  if(! g_signal_connect (overlay_comp, "caps-changed",G_CALLBACK (OverlayState__prepare_overlay), self->overlay_state)){
    C_ERROR ("overlay caps-changed callback Fail...");
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
static void on_rtsp_pad_added (GstElement *element, GstPad *new_pad, RtspPlayer * data){
  C_DEBUG ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (element));

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

  //TODO perform stream selection by stream codec not payload
  if ((payload_v >= 96 && payload_v <=100) || payload_v == 26) {
    data->pad_found = 1;
    data->no_video = 0;
    data->video_done = 0;
    gst_event_new_flush_stop(0);
    GstElement * video_bin = create_video_bin(data);

    gst_bin_add_many (GST_BIN (data->pipeline), video_bin, NULL);

    sink_pad = gst_element_get_static_pad (video_bin, "bin_sink");

    pad_ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (pad_ret)) {
      g_printerr ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(video_bin));
      //TODO Show error on canvas
      data->no_video = 1;
      goto exit;
    }

    gst_element_sync_state_with_parent(video_bin);
  } else if (payload_v == 0) {
    data->pad_found = 1;
    data->no_audio = 0;
    data->audio_done = 0;
    const gchar * audio_format_v = gst_structure_get_string(new_pad_struct,"encoding-name");
    if(!strcmp(audio_format_v,"PCMU")){
      GstElement * audio_bin = create_audio_bin(data);

      gst_bin_add_many (GST_BIN (data->pipeline), audio_bin, NULL);

      sink_pad = gst_element_get_static_pad (audio_bin, "bin_sink");
      pad_ret = gst_pad_link (new_pad, sink_pad);
      if (GST_PAD_LINK_FAILED (pad_ret)) {
        g_printerr ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(audio_bin));
        data->no_audio = 1;
        goto exit;
      }

      gst_element_sync_state_with_parent(audio_bin);
    } else {
      GST_FIXME("Add support to other audio format...\n");
    }
  } else {
    GST_FIXME("Support other payload formats %i\n",payload_v);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);
  if (sink_pad != NULL)
    gst_object_unref (sink_pad);
}

void create_pipeline(RtspPlayer *self){

  self->playing = 0;
  self->audio_done = 0;
  self->no_audio = 1;
  self->video_done = 0;
  self->no_video = 1;
  self->pad_found = 0;

  /* Create the empty pipeline */
  self->pipeline = gst_pipeline_new ("onvif-pipeline");

  /* Create the elements */
  self->src = gst_element_factory_make ("rtspsrc", "rtspsrc");

  if (!self->pipeline){
    C_FATAL("Failed to created pipeline. Check your gstreamer installation...\n");
    return;
  }
  if (!self->src){
    C_FATAL ("Failed to created rtspsrc. Check your gstreamer installation...\n");
    return;
  }

  // Add Elements to the Bin
  gst_bin_add_many (GST_BIN (self->pipeline), self->src, NULL);

  // Dynamic Pad Creation
  if(! g_signal_connect (self->src, "pad-added", G_CALLBACK (on_rtsp_pad_added),self)){
      C_ERROR ("Linking part (1) with part (A)-1 Fail...");
  }

  if(!g_signal_connect (self->src, "select-stream", G_CALLBACK (RtspBackchannel__find),self->backchannel)){
      C_ERROR ("Fail to connect select-stream signal...");
  }

  // g_object_set (G_OBJECT (self->src), "buffer-mode", 3, NULL);
  g_object_set (G_OBJECT (self->src), "latency", 0, NULL);
  g_object_set (G_OBJECT (self->src), "teardown-timeout", 0, NULL); 
  g_object_set (G_OBJECT (self->src), "backchannel", self->enable_backchannel, NULL);
  g_object_set (G_OBJECT (self->src), "user-agent", "OnvifDeviceManager-Linux-0.0", NULL);
  g_object_set (G_OBJECT (self->src), "do-retransmission", FALSE, NULL);
  // g_object_set (G_OBJECT (self->src), "onvif-mode", TRUE, NULL); //It seems onvif mode can cause segmentation fault with v4l2onvif
  g_object_set (G_OBJECT (self->src), "is-live", TRUE, NULL);
  // g_object_set (G_OBJECT (self->src), "tcp-timeout", 500000, NULL);

  /* set up bus */
  GstBus *bus = gst_element_get_bus (self->pipeline);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", G_CALLBACK (message_handler), self);
  gst_object_unref (bus);
}

void RtspPlayer__init(RtspPlayer* self) {
  self->port_fallback = NULL;
  self->user = NULL;
  self->pass = NULL;
  self->location = NULL;
  self->retry_user_data = NULL;
  self->retry_callback = NULL;
  self->error_user_data = NULL;
  self->error_callback = NULL;
  self->stopped_user_data = NULL;
  self->stopped_callback = NULL;
  self->start_user_data = NULL;
  self->start_callback = NULL;
  self->enable_backchannel = 1;
  self->retry = 0;
  self->playing = 0;
  self->allow_overscale = 0;
  self->overlay_state = OverlayState__create();
  self->canvas_handle = NULL;
  self->canvas = NULL;
  self->video_bin = NULL;
  self->audio_bin = NULL;
  self->sink = NULL;
  self->src = NULL;

  P_MUTEX_SETUP(self->prop_lock);
  P_MUTEX_SETUP(self->player_lock);

  self->backchannel = RtspBackchannel__create();

  create_pipeline(self);
  if (!self->pipeline){
    g_error ("Failed to parse pipeline");
    return;
  }
}

RtspPlayer * RtspPlayer__create() {
  RtspPlayer *result  =  (RtspPlayer *) malloc(sizeof(RtspPlayer));
  RtspPlayer__init(result);
  return result;
}

void RtspPlayer__destroy(RtspPlayer* self) {
  if (self) {
    //Making sure stream is stopped
    RtspPlayer__stop(self);

    //Waiting for the state to finish changing
    // GstState current_state_pipe;
    // GstState current_state_back;
    // int ret_1 = gst_element_get_state (self->pipeline,&current_state_pipe, NULL, GST_CLOCK_TIME_NONE);
    // int ret_2 = RtspBackchannel__get_state(self->backchannel, &current_state_back, NULL, GST_CLOCK_TIME_NONE);
    // while( (ret_1 && current_state_pipe != GST_STATE_NULL) || ( ret_2 && current_state_pipe != GST_STATE_NULL)){
    //   C_DEBUG("Waiting for player to stop...\n");
    //   ret_1 = gst_element_get_state (self->pipeline,&current_state_pipe, NULL, GST_CLOCK_TIME_NONE);
    //   ret_2 = RtspBackchannel__get_state(self->backchannel, &current_state_back, NULL, GST_CLOCK_TIME_NONE);
    //   sleep(0.25);
    // }

    if(GST_IS_ELEMENT(self->pipeline)){
      gst_object_unref (self->pipeline);
    }
    
    RtspBackchannel__destroy(self->backchannel);
    OverlayState__destroy(self->overlay_state);

    if(self->location){
      free(self->location);
    }
    if(self->port_fallback){
      free(self->port_fallback);
    }
    if(self->user){
      free(self->user);
    }
    if(self->pass){
      free(self->pass);
    }
    P_MUTEX_CLEANUP(self->prop_lock);
    P_MUTEX_CLEANUP(self->player_lock);
    free(self);
  }
}

void RtspPlayer__set_start_callback(RtspPlayer* self, void (*start_callback)(RtspPlayer *, void *), void * user_data){
  self->start_user_data = user_data;
  self->start_callback = start_callback;
}

void RtspPlayer__set_stopped_callback(RtspPlayer* self, void (*stopped_callback)(RtspPlayer *, void *), void * user_data){
  self->stopped_user_data = user_data;
  self->stopped_callback = stopped_callback;
}

void RtspPlayer__set_error_callback(RtspPlayer* self, void (*error_callback)(RtspPlayer *, void *), void * user_data){
  self->error_user_data = user_data;
  self->error_callback = error_callback;
}

void RtspPlayer__set_retry_callback(RtspPlayer* self, void (*retry_callback)(RtspPlayer *, void *), void * user_data){
  self->retry_user_data = user_data;
  self->retry_callback = retry_callback;
}

void RtspPlayer__set_playback_url(RtspPlayer* self, char *url) {
  P_MUTEX_LOCK(self->prop_lock);
  C_INFO("set location : %s\n",url);

  if(!self->location){
    self->location = malloc(strlen(url)+1);
  } else {
    self->location = realloc(self->location,strlen(url)+1);
  }
  strcpy(self->location,url);
  
  //New location, new credentials
  //TODO Fetch cached credentials from store
  free(self->user);
  free(self->pass);
  self->pass = NULL;
  self->user = NULL;

  P_MUTEX_UNLOCK(self->prop_lock);
}

void RtspPlayer__set_port_fallback(RtspPlayer* self, char * port){
  P_MUTEX_LOCK(self->prop_lock);
  if(!self->port_fallback){
    self->port_fallback = malloc(strlen(port)+1);
  } else {
    self->port_fallback = realloc(self->port_fallback,strlen(port)+1);
  }
  strcpy(self->port_fallback,port);
  P_MUTEX_UNLOCK(self->prop_lock);
}

char * RtspPlayer__get_playback_url(RtspPlayer* self){
  char * ret;
  P_MUTEX_LOCK(self->prop_lock);
  ret = malloc(strlen(self->location)+1);
  strcpy(ret,self->location);
  P_MUTEX_UNLOCK(self->prop_lock);
  return ret;
}

void RtspPlayer__set_credentials(RtspPlayer * self, char * user, char * pass){
  if(self){
    P_MUTEX_LOCK(self->prop_lock);

    if(!user){
      free(self->user);
      self->user = NULL;
    } else {
      if(!self->user){
        self->user = malloc(strlen(user)+1);
      } else {
        self->user = realloc(self->user,strlen(user)+1);
      }
      strcpy(self->user,user);
    }

    if(!pass){
      free(self->pass);
      self->pass = NULL;
    } else {
      if(!self->pass){
        self->pass = malloc(strlen(pass)+1);
      } else {
        self->pass = realloc(self->pass,strlen(pass)+1);
      }
      strcpy(self->pass,pass);
    }

    P_MUTEX_UNLOCK(self->prop_lock);
  }
}

void _RtspPlayer__stop(RtspPlayer* self, int notify){
    C_INFO("RtspPlayer__stop\n");
    if(!self || !self->playing){
      return;
    }

    P_MUTEX_LOCK(self->player_lock);

    self->playing = 0;

    GstStateChangeReturn ret;

    //Pause backchannel
    if(!RtspBackchannel__pause(self->backchannel)){
      goto stop_out;
    }

    //Set NULL state to clear buffers
    if(GST_IS_ELEMENT(self->pipeline)){
      ret = gst_element_set_state (self->pipeline, GST_STATE_NULL);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          C_ERROR ("Unable to set the pipeline to the ready state.\n");
          goto stop_out;
      }
    }
stop_out: 
  //Destroy old pipeline
  if(GST_IS_ELEMENT(self->pipeline))
    gst_object_unref (self->pipeline);

  // Create new pipeline
  create_pipeline(self);

  //New pipeline causes previous pipe to stop dispatching state change.
  //Force hide the previous stream
  if(GTK_IS_WIDGET (self->canvas))
    gtk_widget_set_visible(self->canvas, FALSE);
  //Send stopped signal
  if(notify && self->stopped_callback){
    (*(self->stopped_callback))(self, self->stopped_user_data);
  }
  P_MUTEX_UNLOCK(self->player_lock);
}

void RtspPlayer__stop(RtspPlayer* self){
  _RtspPlayer__stop(self,1);
}

void _RtspPlayer__play(RtspPlayer* self, int retry){
  P_MUTEX_LOCK(self->player_lock);

  //Restore previous session's properties
  P_MUTEX_LOCK(self->prop_lock);
  if(self->user)
    g_object_set (G_OBJECT (self->src), "user-id", self->user, NULL);
  if(self->pass)
    g_object_set (G_OBJECT (self->src), "user-pw", self->pass, NULL);
  if(self->location)
    g_object_set (G_OBJECT (self->src), "location", self->location, NULL);
  P_MUTEX_UNLOCK(self->prop_lock);

  C_INFO("RtspPlayer__play retry[%i] - playing[%i]\n",retry,self->playing);
  if(retry && self->playing == 0){//Retry signal after stop requested
    goto exit;
  } else {
    self->playing = 1;
  }
  
  //Send start signal
  if(self->start_callback){
    (*(self->start_callback))(self, self->start_user_data);
  }

  GstStateChangeReturn ret;
  ret = gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
      C_ERROR ("Unable to set the pipeline to the playing state.\n");
      gst_object_unref (self->pipeline);
      goto exit;
  }

exit:
  P_MUTEX_UNLOCK(self->player_lock);
}

void RtspPlayer__play(RtspPlayer* self){
  self->retry = 0;
  self->enable_backchannel = 1;//TODO Handle parameter input...
  _RtspPlayer__play(self,self->retry);
}

void RtspPlayer__allow_overscale(RtspPlayer * self, int allow_overscale){
  self->allow_overscale = allow_overscale;
  if(GST_IS_GTK_BASE_CUSTOM_SINK(self->sink)){
    gst_gtk_base_custom_sink_set_expand(GST_GTK_BASE_CUSTOM_SINK(self->sink),allow_overscale);
  }
}

/*
Compared to play, retry is design to work after a stream failure.
Stopping will essentially break the retry method and stop the loop.
*/
void RtspPlayer__retry(RtspPlayer* self){
  _RtspPlayer__play(self,self->retry);
}

GtkWidget * OnvifDevice__createCanvas(RtspPlayer *self){

  self->canvas_handle = gtk_grid_new ();
  return self->canvas_handle;
}