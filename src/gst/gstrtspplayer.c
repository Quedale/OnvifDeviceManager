#include "gstrtspplayer.h"
#include "gtk/gstgtkbasesink.h"
#include "clogger.h"
#include "overlay.h"
#include "backchannel.h"
#include "portable_thread.h"
#include "gst/rtsp/gstrtsptransport.h"
#include "url_parser.h"

typedef enum {
    RTSP_FALLBACK_NONE,
    RTSP_FALLBACK_PORT,
    RTSP_FALLBACK_HOST,
    RTSP_FALLBACK_URL
} GstRtspPlayerFallbackType;

typedef struct {
    GstRtspPlayer * owner;
    GstElement *pipeline;
    GstElement *src;  /* RtspSrc to support backchannel */
    GstElement *sink;  /* Video Sink */
    GList * dynamic_elements;
    GstRtspPlayerFallbackType fallback;
    
    //Backpipe related properties
    int enable_backchannel;
    RtspBackchannel * backchannel;
    GstVideoOverlay *overlay; //Overlay rendered on the canvas widget
    OverlayState *overlay_state;

    //Keep location to used on retry
    char * location_set;
    char * location;
    //Retry count
    int retry;
    //Playing or trying to play
    int playing;

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
    char * host_fallback;
} GstRtspPlayerPrivate;

enum {
  STOPPED,
  STARTED,
  RETRY,
  ERROR,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(GstRtspPlayer, GstRtspPlayer_, G_TYPE_OBJECT)

static void 
GstRtspPlayerPrivate__message_handler (GstBus * bus, GstMessage * message, GstRtspPlayerPrivate * priv);
void GstRtspPlayerPrivate__stop(GstRtspPlayerPrivate * priv);

static void
GstRtspPlayer__dispose (GObject *gobject)
{
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (GST_RTSPPLAYER (gobject));

    //Making sure stream is stopped
    GstRtspPlayerPrivate__stop(priv);

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

    if(GST_IS_ELEMENT(priv->pipeline)){
        gst_object_unref (priv->pipeline);
    }

    RtspBackchannel__destroy(priv->backchannel);
    OverlayState__destroy(priv->overlay_state);

    if(priv->location){
        free(priv->location);
    }
    if(priv->location_set){
        free(priv->location_set);
    }
    if(priv->port_fallback){
        free(priv->port_fallback);
    }
    if(priv->host_fallback){
        free(priv->host_fallback);
    }
    if(priv->user){
        free(priv->user);
    }
    if(priv->pass){
        free(priv->pass);
    }
    P_MUTEX_CLEANUP(priv->prop_lock);
    P_MUTEX_CLEANUP(priv->player_lock);

    /* Always chain up to the parent class; there is no need to check if
    * the parent class implements the dispose() virtual function: it is
    * always guaranteed to do so
    */
    G_OBJECT_CLASS (GstRtspPlayer__parent_class)->dispose (gobject);
}

static void
GstRtspPlayer__class_init (GstRtspPlayerClass * klass)
{

    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = GstRtspPlayer__dispose;
    
    signals[STOPPED] =
        g_signal_newv ("stopped",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                0     /* n_params */,
                NULL  /* param_types */);

    signals[STARTED] =
        g_signal_newv ("started",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                0     /* n_params */,
                NULL  /* param_types */);

    signals[RETRY] =
        g_signal_newv ("retry",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                0     /* n_params */,
                NULL  /* param_types */);

    signals[ERROR] =
        g_signal_newv ("error",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                0     /* n_params */,
                NULL  /* param_types */);

}

void GstRtspPlayer__stop(GstRtspPlayer* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    GstRtspPlayerPrivate__stop(priv);
}

void GstRtspPlayerPrivate__play(GstRtspPlayerPrivate* priv){
    P_MUTEX_LOCK(priv->player_lock);

    //Restore previous session's properties
    P_MUTEX_LOCK(priv->prop_lock);
    if(priv->user)
        g_object_set (G_OBJECT (priv->src), "user-id", priv->user, NULL);
    if(priv->pass)
        g_object_set (G_OBJECT (priv->src), "user-pw", priv->pass, NULL);
    if(priv->location)
        g_object_set (G_OBJECT (priv->src), "location", priv->location, NULL);
    P_MUTEX_UNLOCK(priv->prop_lock);

    C_DEBUG("RtspPlayer__play retry[%i] - playing[%i]\n",priv->retry,priv->playing);
    priv->playing = 1;

    GstStateChangeReturn ret;
    ret = gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        C_ERROR ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (priv->pipeline);
        goto exit;
    }

exit:
    P_MUTEX_UNLOCK(priv->player_lock);
}

void GstRtspPlayer__play(GstRtspPlayer* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);

    priv->retry = 0;
    priv->fallback = RTSP_FALLBACK_NONE;
    priv->enable_backchannel = 1;//TODO Handle parameter input...
    GstRtspPlayerPrivate__play(priv);
}

/*
Compared to play, retry is design to work after a stream failure.
Stopping will essentially break the retry method and stop the loop.
*/
void GstRtspPlayer__retry(GstRtspPlayer* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    GstRtspPlayerPrivate__play(priv);
}

/* Dynamically link */
static void 
on_decoder_pad_added (GstElement *element, GstPad *new_pad, gpointer data){
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

static GstElement*
GstRtspPlayerPrivate__create_video_pad(GstRtspPlayerPrivate * priv){
    GstElement *vdecoder, *videoconvert, *overlay_comp, *video_bin;
    GstPad *pad, *ghostpad;

    video_bin = gst_bin_new("video_bin");
    vdecoder = gst_element_factory_make ("decodebin3", NULL);
    videoconvert = gst_element_factory_make ("videoconvert", NULL);
    overlay_comp = gst_element_factory_make ("overlaycomposition", NULL);
    priv->sink = gst_element_factory_make ("gtkcustomsink", NULL);
    gst_base_sink_set_qos_enabled(GST_BASE_SINK_CAST(priv->sink),FALSE);
    gst_base_sink_set_sync(GST_BASE_SINK_CAST(priv->sink),FALSE);
    gst_gtk_base_custom_sink_set_expand(GST_GTK_BASE_CUSTOM_SINK(priv->sink),priv->allow_overscale);

    if (!video_bin ||
            !vdecoder ||
            !videoconvert ||
            !overlay_comp ||
            !priv->sink) {
        C_ERROR ("One of the video elements wasn't created... Exiting\n");
        return NULL;
    }

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (video_bin),
        vdecoder,
        videoconvert,
        overlay_comp,
        priv->sink, NULL);

    // Link confirmation
    if (!gst_element_link_many (videoconvert,
            overlay_comp,
            priv->sink, NULL)){
        C_WARN ("Linking video part (A)-2 Fail...");
        return NULL;
    }

    // Dynamic Pad Creation
    if(! g_signal_connect (vdecoder, "pad-added", G_CALLBACK (on_decoder_pad_added),videoconvert)){
        C_WARN ("Linking (A)-1 part with part (A)-2 Fail...");
    }

    pad = gst_element_get_static_pad (vdecoder, "sink");
    if (!pad) {
        // TODO gst_object_unref 
        C_ERROR("unable to get decoder static sink pad");
        return NULL;
    }

    if(! g_signal_connect (overlay_comp, "draw", G_CALLBACK (OverlayState__draw_overlay), priv->overlay_state)){
        C_ERROR ("overlay draw callback Fail...");
    }

    if(! g_signal_connect (overlay_comp, "caps-changed",G_CALLBACK (OverlayState__prepare_overlay), priv->overlay_state)){
        C_ERROR ("overlay caps-changed callback Fail...");
        return NULL;
    }

    ghostpad = gst_ghost_pad_new ("bin_sink", pad);
    gst_element_add_pad (video_bin, ghostpad);
    gst_object_unref (pad);

    if(!priv->canvas){
        priv->canvas = gst_gtk_base_custom_sink_acquire_widget(GST_GTK_BASE_CUSTOM_SINK(priv->sink));
    } else {
        gst_gtk_base_custom_sink_set_widget(GST_GTK_BASE_CUSTOM_SINK(priv->sink),GTK_GST_BASE_CUSTOM_WIDGET(priv->canvas));
    }
    gst_gtk_base_custom_sink_set_parent(GST_GTK_BASE_CUSTOM_SINK(priv->sink),priv->canvas_handle);

    return video_bin;
}

static GstElement* 
GstRtspPlayerPrivate__create_audio_pad(){
    GstPad *pad, *ghostpad;
    GstElement *decoder, *convert, *level, *sink, *audio_bin;

    audio_bin = gst_bin_new("audiobin");
    decoder = gst_element_factory_make ("decodebin3", NULL);
    convert = gst_element_factory_make ("audioconvert", NULL);
    level = gst_element_factory_make("level",NULL);
    sink = gst_element_factory_make ("autoaudiosink", NULL);
    g_object_set (G_OBJECT (sink), "sync", FALSE, NULL);
    if (!audio_bin ||
            !decoder ||
            !convert ||
            !level ||
            !sink) {
        C_ERROR ("One of the video elements wasn't created... Exiting\n");
        return NULL;
    }

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (audio_bin),
        decoder,
        convert,
        level,
        sink, NULL);


    // Link confirmation
    if (!gst_element_link_many (convert,
            level,
            sink, NULL)){
        C_WARN ("Linking audio part (A)-2 Fail...");
        return NULL;
    }

    // Dynamic Pad Creation
    if(! g_signal_connect (decoder, "pad-added", G_CALLBACK (on_decoder_pad_added),convert)){
        C_WARN ("Linking (A)-1 part (2) with part (A)-2 Fail...");
    }

    pad = gst_element_get_static_pad (decoder, "sink");
    if (!pad) {
        // TODO gst_object_unref 
        C_ERROR("unable to get decoder static sink pad");
        return NULL;
    }

    ghostpad = gst_ghost_pad_new ("bin_sink", pad);
    gst_element_add_pad (audio_bin, ghostpad);
    gst_object_unref (pad);

    return audio_bin;
}

static void
GstRtspPlayerPrivate__on_rtsp_pad_added (GstElement *element, GstPad *new_pad, GstRtspPlayerPrivate * priv){
    C_DEBUG ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (element));
    
    GstPadLinkReturn pad_ret;
    GstPad *sink_pad = NULL;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    char *capsName;
    
    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    capsName = gst_caps_to_string(new_pad_caps);

    // gchar * caps_str = gst_caps_serialize(new_pad_caps,0);
    // gchar * new_pad_type = gst_structure_get_name (new_pad_struct);
    // printf("caps_str : %s\n",caps_str);

    //TODO perform stream selection by stream codec not payload
    if (g_strrstr(capsName, "video")){
        GstElement * video_bin = GstRtspPlayerPrivate__create_video_pad(priv);

        gst_bin_add_many (GST_BIN (priv->pipeline), video_bin, NULL);

        sink_pad = gst_element_get_static_pad (video_bin, "bin_sink");

        pad_ret = gst_pad_link (new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED (pad_ret)) {
            C_ERROR ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(video_bin));
            //TODO Show error on canvas
            goto exit;
        }
        priv->dynamic_elements = g_list_append(priv->dynamic_elements, video_bin);
        gst_element_sync_state_with_parent(video_bin);
    } else if (g_strrstr(capsName,"audio")){
        GstElement * audio_bin = GstRtspPlayerPrivate__create_audio_pad();

        gst_bin_add_many (GST_BIN (priv->pipeline), audio_bin, NULL);

        sink_pad = gst_element_get_static_pad (audio_bin, "bin_sink");
        pad_ret = gst_pad_link (new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED (pad_ret)) {
            C_ERROR ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(audio_bin));
            goto exit;
        }
        priv->dynamic_elements = g_list_append(priv->dynamic_elements, audio_bin);
        gst_element_sync_state_with_parent(audio_bin);
    } else {
        new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
        gint payload_v;
        gst_structure_get_int(new_pad_struct,"payload", &payload_v);
        C_ERROR("Support other payload formats %i\n",payload_v);
    }

exit:
    free(capsName);
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps)
        gst_caps_unref (new_pad_caps);
    if (sink_pad)
        gst_object_unref (sink_pad);
}

static void
GstRtspPlayerPrivate__setup_pipeline (GstRtspPlayerPrivate * priv)
{
    priv->playing = 0;

    /* Create the empty pipeline */
    priv->pipeline = gst_pipeline_new ("onvif-pipeline");

    /* Create the elements */
    priv->src = gst_element_factory_make ("rtspsrc", "rtspsrc");

    if (!priv->pipeline){
        C_FATAL("Failed to created pipeline. Check your gstreamer installation...\n");
        return;
    }
    if (!priv->src){
        C_FATAL ("Failed to created rtspsrc. Check your gstreamer installation...\n");
        return;
    }

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (priv->pipeline), priv->src, NULL);

    // Dynamic Pad Creation
    if(! g_signal_connect (priv->src, "pad-added", G_CALLBACK (GstRtspPlayerPrivate__on_rtsp_pad_added),priv)){
        C_ERROR ("Linking part (1) with part (A)-1 Fail...");
    }

    if(!g_signal_connect (priv->src, "select-stream", G_CALLBACK (RtspBackchannel__find),priv->backchannel)){
        C_ERROR ("Fail to connect select-stream signal...");
    }

    // g_object_set (G_OBJECT (priv->src), "buffer-mode", 3, NULL);
    g_object_set (G_OBJECT (priv->src), "latency", 0, NULL);
    g_object_set (G_OBJECT (priv->src), "teardown-timeout", 0, NULL); 
    g_object_set (G_OBJECT (priv->src), "backchannel", priv->enable_backchannel, NULL);
    g_object_set (G_OBJECT (priv->src), "user-agent", "OnvifDeviceManager-Linux-0.0", NULL);
    g_object_set (G_OBJECT (priv->src), "do-retransmission", TRUE, NULL);
    g_object_set (G_OBJECT (priv->src), "onvif-mode", FALSE, NULL); //It seems onvif mode can cause segmentation fault with v4l2onvif
    g_object_set (G_OBJECT (priv->src), "is-live", TRUE, NULL);
    g_object_set (G_OBJECT (priv->src), "tcp-timeout", 1000000, NULL);
    g_object_set (G_OBJECT (priv->src), "protocols", GST_RTSP_LOWER_TRANS_TCP, NULL); //TODO Allow changing this via settings

    /* set up bus */
    GstBus *bus = gst_element_get_bus (priv->pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (GstRtspPlayerPrivate__message_handler), priv);
    gst_object_unref (bus);
}

void GstRtspPlayerPrivate__inner_stop(GstRtspPlayerPrivate * priv){
    GstStateChangeReturn ret;

    //Pause backchannel
    if(!RtspBackchannel__pause(priv->backchannel)){
        return;
    }

    //Set NULL state to clear buffers
    if(GST_IS_ELEMENT(priv->pipeline)){
        ret = gst_element_set_state (priv->pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            C_ERROR ("Unable to set the pipeline to the ready state.\n");
            return;
        }
    }

    g_list_free(priv->dynamic_elements);
    priv->dynamic_elements = NULL;

    //Destroy old pipeline
    if(GST_IS_ELEMENT(priv->pipeline))
        gst_object_unref (priv->pipeline);

    // Create new pipeline
    GstRtspPlayerPrivate__setup_pipeline(priv);

    //New pipeline causes previous pipe to stop dispatching state change.
    //Force hide the previous stream
    if(GTK_IS_WIDGET (priv->canvas))
        gtk_widget_set_visible(priv->canvas, FALSE);

}

void GstRtspPlayerPrivate__stop(GstRtspPlayerPrivate * priv){
    C_DEBUG("RtspPlayer__stop\n");
    
    P_MUTEX_LOCK(priv->player_lock);

    priv->playing = 0;

    GstRtspPlayerPrivate__inner_stop(priv);

    g_signal_emit (priv->owner, signals[STOPPED], 0 /* details */);
    P_MUTEX_UNLOCK(priv->player_lock);
}

static void 
GstRtspPlayerPrivate__warning_msg (GstBus *bus, GstMessage *msg) {
    GError *err;
    gchar *debug_info;
    gst_message_parse_warning (msg, &err, &debug_info);
    if(strcmp(err->message,"Empty Payload.")){ //Ignoring rtp empty payload warnings
        C_WARN ("Warning received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
        C_WARN ("Debugging information: %s", debug_info ? debug_info : "none");
    }
    g_clear_error (&err);
    g_free (debug_info);
}

static void GstRtspPlayerPrivate__process_fallback(GstRtspPlayerPrivate * priv){
    switch(priv->fallback){
        case RTSP_FALLBACK_NONE:
            priv->fallback = RTSP_FALLBACK_HOST;
            char * host_fallback = NULL;
            if(priv->host_fallback) host_fallback = URL__set_host(priv->location_set, priv->host_fallback);
            if(host_fallback && strcmp(host_fallback,priv->location_set) != 0){
                C_WARN("Attempting URL host correction : [%s] --> [%s]",priv->location_set, host_fallback);
                free(priv->location);
                priv->location = host_fallback;
                break;
            } else {
                free(host_fallback);
            }
            __attribute__ ((fallthrough));
        case RTSP_FALLBACK_HOST:
            priv->fallback = RTSP_FALLBACK_PORT;
            char * port_fallback = NULL;
            if(priv->port_fallback) port_fallback = URL__set_port(priv->location_set, priv->port_fallback);
            if(port_fallback && strcmp(port_fallback,priv->location_set) != 0){
                C_WARN("Attempting URL port correction : [%s] --> [%s]",priv->location_set, port_fallback);
                free(priv->location);
                priv->location = port_fallback;
                break;
            } else {
                free(port_fallback);
            }
            __attribute__ ((fallthrough));
        case RTSP_FALLBACK_PORT:
            priv->fallback = RTSP_FALLBACK_URL;
            char * tmp_fallback = NULL;
            char * url_fallback = NULL;
            if(priv->host_fallback && priv->port_fallback){
                tmp_fallback = URL__set_port(priv->location_set, priv->port_fallback);
                url_fallback = URL__set_host(tmp_fallback, priv->host_fallback);
            }
            if(url_fallback && strcmp(url_fallback,priv->location_set) != 0){
                C_WARN("Attempting root URL correction : [%s] --> [%s]",priv->location_set, url_fallback);
                free(priv->location);
                priv->location = url_fallback;
                free(tmp_fallback);
                break;
            } else {
                free(url_fallback);
                free(tmp_fallback);
            }
            __attribute__ ((fallthrough));
        case RTSP_FALLBACK_URL:
        default:
            //Do not retry after connection failure
            priv->playing = 0;
            C_ERROR ("Failed to connect to %s", priv->location_set);
            break;
    }
}

/* This function is called when an error message is posted on the bus */
static void 
GstRtspPlayerPrivate__error_msg (GstBus *bus, GstMessage *msg, GstRtspPlayerPrivate * priv) {
    GError *err;
    gchar *debug_info;
    
    int fallback = 0;

    P_MUTEX_LOCK(priv->player_lock);

    gst_message_parse_error (msg, &err, &debug_info);

    switch(err->code){
        case GST_RESOURCE_ERROR_SETTINGS:
            C_WARN ("Backchannel unsupported. Downgrading...");
            if(priv->enable_backchannel){
                priv->enable_backchannel = 0;
                priv->retry--; //This doesn't count as a try. Finding out device capabilities count has handshake
            } else {
                C_ERROR ("Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
                C_ERROR ("Debugging information: %s", debug_info ? debug_info : "none");
                C_ERROR ("Error code : %d",err->code);
            }
            break;
        case GST_RESOURCE_ERROR_OPEN_READ:
        case GST_RESOURCE_ERROR_OPEN_WRITE:
        case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
            if(priv->port_fallback || priv->host_fallback){
                fallback = 1;
                GstRtspPlayerPrivate__process_fallback(priv);
            } else {
                C_ERROR ("Failed to connect to %s", priv->location_set);
            }
            break;
        case GST_RESOURCE_ERROR_NOT_AUTHORIZED:
        case GST_RESOURCE_ERROR_NOT_FOUND:
            C_ERROR("Non-recoverable error encountered.");
            priv->playing = 0;
            __attribute__ ((fallthrough));
        default:
            C_ERROR ("Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
            C_ERROR ("Debugging information: %s", debug_info ? debug_info : "none");
            C_ERROR ("Error code : %d",err->code);
    }

    g_clear_error (&err);
    g_free (debug_info);

    if((fallback && priv->playing) || (priv->retry < 3 && priv->playing)){ //TODO Check if any retry callback exists
        //Stopping player after if condition because "playing" gets reset
        GstRtspPlayerPrivate__inner_stop(priv);
        C_WARN("****************************************************");
        if(fallback) C_WARN("* URI fallback attempt %s", priv->location); else C_WARN("* Retry attempt #%i - %s",priv->retry, priv->location);
        C_WARN("****************************************************");
        P_MUTEX_UNLOCK(priv->player_lock);
        //Retry signal - The player doesn't invoke retry on its own to allow the invoker to dispatch it asynchroniously
        if(!fallback)
            priv->retry++;
        g_signal_emit (priv->owner, signals[RETRY], 0 /* details */);
    } else if(priv->playing == 1) { //Ignoring error after the player requested to stop (gst_rtspsrc_try_send)
        priv->playing = 0;
        GstRtspPlayerPrivate__inner_stop(priv);
        P_MUTEX_UNLOCK(priv->player_lock);
        //Error signal
        g_signal_emit (priv->owner, signals[ERROR], 0 /* details */);
    } else {
        P_MUTEX_UNLOCK(priv->player_lock);
    }
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void 
GstRtspPlayerPrivate__eos_msg (GstBus *bus, GstMessage *msg, GstRtspPlayerPrivate * priv) {
    C_ERROR ("End-Of-Stream reached.\n");
    GstRtspPlayerPrivate__stop(priv);
}

int GstRtspPlayerPrivate__is_dynamic_pad(GstRtspPlayerPrivate * priv, GstElement * element){
    if(g_list_length(priv->dynamic_elements) == 0){
        return FALSE;
    }
    GList *node_itr = priv->dynamic_elements;
    while (node_itr != NULL)
    {
        GstElement * bin = (GstElement *) node_itr->data;
        if(bin == element){
            return TRUE;
        }
        node_itr = g_list_next(node_itr);
    }
    return FALSE;
}

static int 
GstRtspPlayerPrivate__is_video_bin(GstRtspPlayerPrivate * priv, GstElement * element){
    if(GST_IS_OBJECT(element) && strcmp(GST_OBJECT_NAME(element),"video_bin") == 0){
        return TRUE;
    }

    return FALSE;
}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void
GstRtspPlayerPrivate__state_changed_msg (GstBus * bus, GstMessage * msg, GstRtspPlayerPrivate * priv)
{
    GstState old_state, new_state, pending_state;

    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    /* Using video_bin for state change since the pipeline is PLAYING before the videosink */
    
    GstElement * element = GST_ELEMENT(GST_MESSAGE_SRC (msg));
    if(GstRtspPlayerPrivate__is_video_bin(priv,element) && new_state != GST_STATE_PLAYING && GTK_IS_WIDGET (priv->canvas)){
        gtk_widget_set_visible(priv->canvas, FALSE);
    } else if(GstRtspPlayerPrivate__is_video_bin(priv,element) && new_state == GST_STATE_PLAYING && GTK_IS_WIDGET (priv->canvas)){
        gtk_widget_set_visible(priv->canvas, TRUE);

        /*
        * Waiting for fix https://gitlab.freedesktop.org/gstreamer/gst-plugins-good/-/issues/245
        * Until This issue is fixed, "no-more-pads" is unreliable to determine if all stream states are ready.
        * Until then, we only rely on the video stream state to fire the started signal
        * Funny enough, a gstreamer generated onvif rtsp stream causes this issue
        * 
        * "select-stream" might be an alternative, although I don't know if the first stream could play before the last one is shown
        */
        priv->retry = 0;
        priv->fallback = RTSP_FALLBACK_NONE;
        g_signal_emit (priv->owner, signals[STARTED], 0 /* details */);
    }

    if(GstRtspPlayerPrivate__is_dynamic_pad(priv,element)){
        C_TRACE ("State set to %s for %s\n", gst_element_state_get_name (new_state), GST_OBJECT_NAME (msg->src));
    }
}


static void 
GstRtspPlayerPrivate__message_handler (GstBus * bus, GstMessage * message, GstRtspPlayerPrivate * priv)
{ 
    const GstStructure *s;
    const gchar *name;
    switch(GST_MESSAGE_TYPE(message)){
        case GST_MESSAGE_UNKNOWN:
            C_TRACE("msg : GST_MESSAGE_UNKNOWN\n");
            break;
        case GST_MESSAGE_EOS:
            GstRtspPlayerPrivate__eos_msg(bus,message,priv);
            break;
        case GST_MESSAGE_ERROR:
            GstRtspPlayerPrivate__error_msg(bus,message,priv);
            break;
        case GST_MESSAGE_WARNING:
            GstRtspPlayerPrivate__warning_msg(bus,message);
            break;
        case GST_MESSAGE_INFO:
            C_TRACE("msg : GST_MESSAGE_INFO\n");
            break;
        case GST_MESSAGE_TAG:
            // tag_cb(bus,message,player);
            break;
        case GST_MESSAGE_BUFFERING:
            C_TRACE("msg : GST_MESSAGE_BUFFERING\n");
            break;
        case GST_MESSAGE_STATE_CHANGED:
            GstRtspPlayerPrivate__state_changed_msg(bus,message,priv);
            break;
        case GST_MESSAGE_STATE_DIRTY:
            C_TRACE("msg : GST_MESSAGE_STATE_DIRTY\n");
            break;
        case GST_MESSAGE_STEP_DONE:
            C_TRACE("msg : GST_MESSAGE_STEP_DONE\n");
            break;
        case GST_MESSAGE_CLOCK_PROVIDE:
            C_TRACE("msg : GST_MESSAGE_CLOCK_PROVIDE\n");
            break;
        case GST_MESSAGE_CLOCK_LOST:
            C_TRACE("msg : GST_MESSAGE_CLOCK_LOST\n");
            break;
        case GST_MESSAGE_NEW_CLOCK:
            break;
        case GST_MESSAGE_STRUCTURE_CHANGE:
            C_TRACE("msg : GST_MESSAGE_STRUCTURE_CHANGE\n");
            break;
        case GST_MESSAGE_STREAM_STATUS:
            break;
        case GST_MESSAGE_APPLICATION:
            C_TRACE("msg : GST_MESSAGE_APPLICATION\n");
            break;
        case GST_MESSAGE_ELEMENT:
            s = gst_message_get_structure (message);
            name = gst_structure_get_name (s);
            if (strcmp (name, "level") == 0) {
                OverlayState__level_handler(bus,message,priv->overlay_state,s);
            } else if (strcmp (name, "GstNavigationMessage") == 0 || 
                        strcmp (name, "application/x-rtp-source-sdes") == 0){
                //Ignore intentionally left unhandled for now
            } else {
                C_ERROR("Unhandled element msg name : %s//%d\n",name,message->type);
            }
            break;
        case GST_MESSAGE_SEGMENT_START:
            C_TRACE("msg : GST_MESSAGE_SEGMENT_START\n");
            break;
        case GST_MESSAGE_SEGMENT_DONE:
            C_TRACE("msg : GST_MESSAGE_SEGMENT_DONE\n");
            break;
        case GST_MESSAGE_DURATION_CHANGED:
            C_TRACE("msg : GST_MESSAGE_DURATION_CHANGED\n");
            break;
        case GST_MESSAGE_LATENCY:
            break;
        case GST_MESSAGE_ASYNC_START:
            C_TRACE("msg : GST_MESSAGE_ASYNC_START\n");
            break;
        case GST_MESSAGE_ASYNC_DONE:
            // printf("msg : GST_MESSAGE_ASYNC_DONE\n");
            break;
        case GST_MESSAGE_REQUEST_STATE:
            C_TRACE("msg : GST_MESSAGE_REQUEST_STATE\n");
            break;
        case GST_MESSAGE_STEP_START:
            C_TRACE("msg : GST_MESSAGE_STEP_START\n");
            break;
        case GST_MESSAGE_QOS:
            break;
        case GST_MESSAGE_PROGRESS:
            break;
        case GST_MESSAGE_TOC:
            C_TRACE("msg : GST_MESSAGE_TOC\n");
            break;
        case GST_MESSAGE_RESET_TIME:
            C_TRACE("msg : GST_MESSAGE_RESET_TIME\n");
            break;
        case GST_MESSAGE_STREAM_START:
            break;
        case GST_MESSAGE_NEED_CONTEXT:
            break;
        case GST_MESSAGE_HAVE_CONTEXT:
            break;
        case GST_MESSAGE_EXTENDED:
            C_TRACE("msg : GST_MESSAGE_EXTENDED\n");
            break;
        case GST_MESSAGE_DEVICE_ADDED:
            C_TRACE("msg : GST_MESSAGE_DEVICE_ADDED\n");
            break;
        case GST_MESSAGE_DEVICE_REMOVED:
            C_TRACE("msg : GST_MESSAGE_DEVICE_REMOVED\n");
            break;
        case GST_MESSAGE_PROPERTY_NOTIFY:
            C_TRACE("msg : GST_MESSAGE_PROPERTY_NOTIFY\n");
            break;
        case GST_MESSAGE_STREAM_COLLECTION:
            C_TRACE("msg : GST_MESSAGE_STREAM_COLLECTION\n");
            break;
        case GST_MESSAGE_STREAMS_SELECTED:
            C_TRACE("msg : GST_MESSAGE_STREAMS_SELECTED\n");
            break;
        case GST_MESSAGE_REDIRECT:
            C_TRACE("msg : GST_MESSAGE_REDIRECT\n");
            break;
        //Doesnt exists in gstreamer 1.16
        // case GST_MESSAGE_DEVICE_CHANGED:
        //  printf("msg : GST_MESSAGE_DEVICE_CHANGED\n");
        //  break;
        // case GST_MESSAGE_INSTANT_RATE_REQUEST:
        //  printf("msg : GST_MESSAGE_INSTANT_RATE_REQUEST\n");
        //  break;
        case GST_MESSAGE_ANY:
            C_WARN("msg : GST_MESSAGE_ANY\n");
            break;
        default:
            C_WARN("msg : default....");
    }
}

static void
GstRtspPlayer__init (GstRtspPlayer * self)
{
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    priv->owner = self;
    priv->port_fallback = NULL;
    priv->host_fallback = NULL;
    priv->user = NULL;
    priv->pass = NULL;
    priv->location = NULL;
    priv->location_set = NULL;
    priv->enable_backchannel = 1;
    priv->retry = 0;
    priv->allow_overscale = 0;
    priv->overlay_state = OverlayState__create();
    priv->canvas_handle = NULL;
    priv->canvas = NULL;
    priv->dynamic_elements = NULL;
    priv->sink = NULL;
    priv->fallback = RTSP_FALLBACK_NONE;

    P_MUTEX_SETUP(priv->prop_lock);
    P_MUTEX_SETUP(priv->player_lock);

    priv->backchannel = RtspBackchannel__create();

    GstRtspPlayerPrivate__setup_pipeline(priv);
    if (!priv->pipeline){
        C_ERROR ("Failed to parse pipeline");
        return;
    }
}

GstRtspPlayer*  GstRtspPlayer__new (){
    GstRtspPlayer * player = g_object_new (GST_TYPE_RTSPPLAYER, NULL);
    return player;
}

void GstRtspPlayer__set_playback_url(GstRtspPlayer * self, char *url) {
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    P_MUTEX_LOCK(priv->prop_lock);
    C_INFO("set location : %s\n",url);
    
    if(!priv->location_set){
        priv->location_set = malloc(strlen(url)+1);
    } else {
        priv->location_set = realloc(priv->location_set,strlen(url)+1);
    }
    strcpy(priv->location_set,url);

    if(!priv->location){
        priv->location = malloc(strlen(url)+1);
    } else {
        priv->location = realloc(priv->location,strlen(url)+1);
    }
    strcpy(priv->location,url);
    
    //New location, new credentials
    //TODO Fetch cached credentials from store
    free(priv->user);
    free(priv->pass);
    priv->pass = NULL;
    priv->user = NULL;

    P_MUTEX_UNLOCK(priv->prop_lock);
}

void GstRtspPlayer__set_credentials(GstRtspPlayer * self, char * user, char * pass){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);

    P_MUTEX_LOCK(priv->prop_lock);

    if(!user){
        free(priv->user);
        priv->user = NULL;
    } else {
        if(!priv->user){
            priv->user = malloc(strlen(user)+1);
        } else {
            priv->user = realloc(priv->user,strlen(user)+1);
        }
        strcpy(priv->user,user);
    }

    if(!pass){
        free(priv->pass);
        priv->pass = NULL;
    } else {
        if(!priv->pass){
            priv->pass = malloc(strlen(pass)+1);
        } else {
            priv->pass = realloc(priv->pass,strlen(pass)+1);
        }
        strcpy(priv->pass,pass);
    }

    P_MUTEX_UNLOCK(priv->prop_lock);
}

GtkWidget * GstRtspPlayer__createCanvas(GstRtspPlayer *self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (GST_IS_RTSPPLAYER (self), NULL);
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);

    priv->canvas_handle = gtk_grid_new ();
    return priv->canvas_handle;
}

gboolean GstRtspPlayer__is_mic_mute(GstRtspPlayer* self) {
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (GST_IS_RTSPPLAYER (self), FALSE);
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    return RtspBackchannel__is_mute(priv->backchannel);
}

void GstRtspPlayer__mic_mute(GstRtspPlayer* self, gboolean mute) {
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    RtspBackchannel__mute(priv->backchannel, mute);
}

void GstRtspPlayer__set_allow_overscale(GstRtspPlayer * self, int allow_overscale){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    priv->allow_overscale = allow_overscale;
    if(GST_IS_GTK_BASE_CUSTOM_SINK(priv->sink)){
        gst_gtk_base_custom_sink_set_expand(GST_GTK_BASE_CUSTOM_SINK(priv->sink),allow_overscale);
    }
}

void GstRtspPlayer__set_port_fallback(GstRtspPlayer* self, char * port){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    P_MUTEX_LOCK(priv->prop_lock);
    if(!priv->port_fallback){
        priv->port_fallback = malloc(strlen(port)+1);
    } else {
        priv->port_fallback = realloc(priv->port_fallback,strlen(port)+1);
    }
    strcpy(priv->port_fallback,port);
    P_MUTEX_UNLOCK(priv->prop_lock);
}

void GstRtspPlayer__set_host_fallback(GstRtspPlayer* self, char * host){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));

    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    P_MUTEX_LOCK(priv->prop_lock);
    if(!priv->host_fallback){
        priv->host_fallback = malloc(strlen(host)+1);
    } else {
        priv->host_fallback = realloc(priv->host_fallback,strlen(host)+1);
    }
    strcpy(priv->host_fallback,host);
    P_MUTEX_UNLOCK(priv->prop_lock);
}