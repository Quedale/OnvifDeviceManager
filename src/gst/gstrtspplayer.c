#include "gstrtspplayer.h"
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


struct _GstRtspPlayerSession {
    GstRtspPlayer * player;
    GstElement *pipeline;
    GstElement *src;  /* RtspSrc to support backchannel */
    GList * dynamic_elements;
    GstRtspPlayerFallbackType fallback;

    //Keep location to used on retry
    char * location_set;
    char * location;
    int valid_location;

    //Retry count
    int retry;
    char * user;
    char * pass;
    char * port_fallback;
    char * host_fallback;
    int enable_backchannel;
    void * user_data;
};

typedef struct {
    GstRtspPlayer * owner;
    GstRtspPlayerSession * session;
    GMainContext * player_context;
    GMainLoop * player_loop;
    P_COND_TYPE loop_cond;
    P_MUTEX_TYPE loop_lock;
    //Reusable bins containing encoder and sink
    GstElement * video_bin;
    GstElement * audio_bin;
    GstElement *sink;  /* Video Sink */
    GstElement *snapsink;
    GstCaps * sinkcaps; /* reference to extract native stream dimension */
    OverlayState *overlay_state;
    //Backpipe related properties
    RtspBackchannel * backchannel;

    //Playing or trying to play
    int playing;

    //Grid holding the canvas
    GtkWidget *canvas_handle;
    //Canvas used to draw stream
    GtkWidget *canvas;
    GstRtspViewMode view_mode;

    P_MUTEX_TYPE player_lock;
} GstRtspPlayerPrivate;

typedef struct {
    GstRtspPlayer * player;
    guint signalid;
    va_list args;

    int done;
    P_COND_TYPE cond;
    P_MUTEX_TYPE lock;
} GstSignalWaitData;

struct GstInitData{
    GMainContext ** context;
    GMainLoop ** loop;
    int done;
    P_COND_TYPE * finish_cond;
    P_COND_TYPE cond;
    P_MUTEX_TYPE lock;
};

enum {
  STOPPED,
  STARTED,
  RETRY,
  ERROR,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(GstRtspPlayer, GstRtspPlayer_, G_TYPE_OBJECT)

static gboolean 
GstRtspPlayerSession__message_handler (GstBus * bus, GstMessage * message, GstRtspPlayerSession * session);

static gboolean _player_signal_and_wait(GstSignalWaitData * data){
    g_signal_emit_valist (data->player, data->signalid, 0, data->args);
    data->done = 1;
    P_COND_BROADCAST(data->cond);
    return FALSE;
}

void player_signal_and_wait(GstRtspPlayer * self, guint signalid, ...){
    GstSignalWaitData data;
    va_start(data.args, signalid);
    data.done = 0;
    data.player = self;
    data.signalid = signalid;
    P_COND_SETUP(data.cond);
    P_MUTEX_SETUP(data.lock);
    g_main_context_invoke(g_main_context_default(),G_SOURCE_FUNC(_player_signal_and_wait),&data);
    if(!data.done) { P_COND_WAIT(data.cond, data.lock); }
    P_COND_CLEANUP(data.cond);
    P_MUTEX_CLEANUP(data.lock);
    va_end(data.args);
}

static GstRtspPlayerSession * GstRtspPlayerSession__create(GstRtspPlayer * player, char * url, char * user, char * pass, char * fallback_host, char * fallback_port, void * user_data){
    GstRtspPlayerSession * session = malloc(sizeof(GstRtspPlayerSession));
    session->player = player;
    session->user_data = user_data;

    //Updating location
    session->location_set = malloc(strlen(url)+1);
    strcpy(session->location_set,url);
    session->location = malloc(strlen(url)+1);
    strcpy(session->location,url);
    session->valid_location = 0;

    //Update credentials
    if(!user){
        session->user = NULL;
    } else {
        session->user = malloc(strlen(user)+1);
        strcpy(session->user,user);
    }

    if(!pass){
        session->pass = NULL;
    } else {
        session->pass = malloc(strlen(pass)+1);
        strcpy(session->pass,pass);
    }

    //Update port fallback
    if(fallback_port){
        session->port_fallback = malloc(strlen(fallback_port)+1);
        strcpy(session->port_fallback,fallback_port);
    } else {
        session->port_fallback = NULL;
    }

    //Update host fallback
    if(fallback_host){
        session->host_fallback = malloc(strlen(fallback_host)+1);
        strcpy(session->host_fallback,fallback_host);
    } else {
        session->host_fallback = NULL;
    }

    session->enable_backchannel = 1;
    session->retry = 0;
    session->dynamic_elements = NULL;
    session->fallback = RTSP_FALLBACK_NONE;

    return session;
}

char * GstRtspPlayerSession__get_uri(GstRtspPlayerSession * session){
    if(!session){
        return NULL;
    }
    return session->location;
}

static void GstRtspPlayerSession__destroy(GstRtspPlayerSession * session){
    C_DEBUG("GstRtspPlayerSession__destroy");
    if(session){
        if(GST_IS_ELEMENT(session->pipeline)){
            gst_object_unref (session->pipeline);
            session->pipeline = NULL;
        }
        if(session->port_fallback){
            free(session->port_fallback);
            session->port_fallback = NULL;
        }
        if(session->host_fallback){
            free(session->host_fallback);
            session->host_fallback = NULL;
        }
        if(session->user){
            free(session->user);
            session->user = NULL;
        }
        if(session->pass){
            free(session->pass);
            session->pass = NULL;
        }
        if(session->location){
            free(session->location);
            session->location = NULL;
        }
        if(session->location_set){
            free(session->location_set);
            session->location_set = NULL;
        }
        free(session);
    }
}

void * GstRtspPlayerSession__get_user_data(GstRtspPlayerSession * session){
    if(!session){
        return NULL;
    }
    return session->user_data;
}

GstRtspPlayerSession *
GstRtspPlayer__get_session (GstRtspPlayer * self)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (GST_IS_RTSPPLAYER (self), NULL);
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    return priv->session;
}

/* Dynamically link */
static void 
on_decoder_pad_added (GstElement *element, GstPad *new_pad, gpointer data){
    GstPad *sinkpad;
    GstPadLinkReturn ret;
    GstElement *decoder = (GstElement *) data;

    /* We can now link this pad with the rtsp-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    ret = gst_pad_link (new_pad, sinkpad);

    if (GST_PAD_LINK_FAILED (ret)) {
        C_ERROR("failed to link dynamically '%s' to '%s'",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(decoder));
    }

    gst_object_unref (sinkpad);
}

void GstRtspPlayerPrivate__apply_view_mode(GstRtspPlayerPrivate * priv){
    switch(priv->view_mode){
        case GST_RTSP_PLAYER_VIEW_MODE_FIT_WINDOW:
            gtk_widget_set_vexpand(GTK_WIDGET(priv->canvas),FALSE);
            gtk_widget_set_hexpand(GTK_WIDGET(priv->canvas),FALSE);
            gtk_widget_set_vexpand(GTK_WIDGET(priv->canvas_handle),FALSE);
            gtk_widget_set_hexpand(GTK_WIDGET(priv->canvas_handle),FALSE);
            gtk_widget_set_size_request(GTK_WIDGET(priv->canvas),-1,-1);
            break;
        case GST_RTSP_PLAYER_VIEW_MODE_FILL_WINDOW:
            gtk_widget_set_vexpand(GTK_WIDGET(priv->canvas),TRUE);
            gtk_widget_set_hexpand(GTK_WIDGET(priv->canvas),TRUE);
            gtk_widget_set_vexpand(GTK_WIDGET(priv->canvas_handle),TRUE);
            gtk_widget_set_hexpand(GTK_WIDGET(priv->canvas_handle),TRUE);
            gtk_widget_set_size_request(GTK_WIDGET(priv->canvas),-1,-1);
            break;
        case GST_RTSP_PLAYER_VIEW_MODE_NATIVE:
            gtk_widget_set_vexpand(GTK_WIDGET(priv->canvas),TRUE);
            gtk_widget_set_hexpand(GTK_WIDGET(priv->canvas),TRUE);
            gtk_widget_set_vexpand(GTK_WIDGET(priv->canvas_handle),TRUE);
            gtk_widget_set_hexpand(GTK_WIDGET(priv->canvas_handle),TRUE);
            if(GST_IS_CAPS(priv->sinkcaps)){
                GstStructure * caps_struct = gst_caps_get_structure (priv->sinkcaps, 0);
                gint width,height;
                gst_structure_get_int(caps_struct,"width",&width);
                gst_structure_get_int(caps_struct,"height",&height);
                gtk_widget_set_size_request(GTK_WIDGET(priv->canvas),width,height);
            }

            break;
        default:
            C_WARN("%s Unsupported setting",priv->session->location);
            break;
    }
}

void GstRtspPlayer__caps_changed_cb (GstElement * overlay, GstCaps * caps, gint window_width, gint window_height, GstRtspPlayerPrivate * priv){
    priv->sinkcaps = caps;
    GstRtspPlayerPrivate__apply_view_mode(priv);
}

static GstElement*
GstRtspPlayerPrivate__create_video_pad(GstRtspPlayerPrivate * priv){
    GstElement *vdecoder, *videoconvert, *overlay_comp, *video_bin;
    GstPad *pad, *ghostpad;

    video_bin = gst_bin_new("video_bin");
    vdecoder = gst_element_factory_make ("decodebin3", "video_decodebin");
    videoconvert = gst_element_factory_make ("videoconvert", "videoconverter");
    overlay_comp = gst_element_factory_make ("overlaycomposition", NULL);
    priv->sink = gst_element_factory_make ("glsinkbin", "glsinkbin");
    priv->snapsink = gst_element_factory_make ("gtkglsink", "gtkglsink");
    if (priv->snapsink != NULL && priv->sink != NULL) {
        C_INFO ("Successfully created GTK GL Sink\n");
        g_object_set (priv->sink, "sink", priv->snapsink, NULL);
        g_object_get (priv->snapsink, "widget", &priv->canvas, NULL);
        //Temporarely disabled for performance
        g_object_set (G_OBJECT (priv->snapsink), "enable-last-sample", FALSE, NULL);

        // gst_base_sink_set_sync(GST_BASE_SINK_CAST(priv->snapsink),FALSE);
        gst_base_sink_set_qos_enabled(GST_BASE_SINK_CAST(priv->snapsink),FALSE);
    } else {
        C_WARN ("Could not create gtkglsink, falling back to gtksink.\n");
        priv->sink = gst_element_factory_make ("gtksink", "gtksink");
        priv->snapsink = priv->sink;
        g_object_get (priv->sink, "widget", &priv->canvas, NULL);
        //Temporarely disabled for performance
        g_object_set (G_OBJECT (priv->snapsink), "enable-last-sample", FALSE, NULL);

        gst_base_sink_set_sync(GST_BASE_SINK_CAST(priv->sink),FALSE);
        gst_base_sink_set_qos_enabled(GST_BASE_SINK_CAST(priv->sink),FALSE);
    }
    gtk_widget_set_no_show_all(priv->canvas, TRUE);
    gtk_container_add (GTK_CONTAINER (priv->canvas_handle), GTK_WIDGET(priv->canvas));

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

    if(! g_signal_connect (overlay_comp, "caps-changed",G_CALLBACK (GstRtspPlayer__caps_changed_cb), priv)){
        C_ERROR ("sink caps-changed callback Fail...");
        return NULL;
    }

    ghostpad = gst_ghost_pad_new ("bin_sink", pad);
    gst_element_add_pad (video_bin, ghostpad);
    gst_object_unref (pad);

    return video_bin;
}

static GstElement* 
GstRtspPlayerPrivate__create_audio_pad(){
    GstPad *pad, *ghostpad;
    GstElement *decoder, *convert, *level, *sink, *audio_bin;

    audio_bin = gst_bin_new("audiobin");
    decoder = gst_element_factory_make ("decodebin3", "audio_decodebin");
    convert = gst_element_factory_make ("audioconvert", "audioconverter");
    level = gst_element_factory_make("level",NULL);
    sink = gst_element_factory_make ("autoaudiosink", NULL);
    g_object_set (G_OBJECT (sink), "sync", FALSE, NULL);
    if (!audio_bin ||
            !decoder ||
            !convert ||
            !level ||
            !sink) {
        C_ERROR ("One of the audio elements wasn't created... Exiting\n");
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

struct create_video_data{
    GstRtspPlayerSession * session;
    GstPad *new_pad;
    GstElement *element;
    P_COND_TYPE cond;
    P_MUTEX_TYPE lock;
    int done;
};

gboolean GstRtspPlayerSession__idle_attach_video_pad(void * user_data){
    struct create_video_data * data = (struct create_video_data *) user_data;
    GstPad *sink_pad;
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (data->session->player);
    gst_bin_add_many (GST_BIN (data->session->pipeline), priv->video_bin, NULL);

    C_TRACE("%s GstRtspPlayerSession__idle_attach_video_pad", data->session->location);
    sink_pad = gst_element_get_static_pad (priv->video_bin, "bin_sink");

    GstPadLinkReturn pad_ret = gst_pad_link (data->new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (pad_ret)) {
        C_ERROR ("%s failed to link dynamically '%s' to '%s'", data->session->location,GST_ELEMENT_NAME(data->element),GST_ELEMENT_NAME(priv->video_bin));
        //TODO Show error on canvas
        goto exit;
    }
    data->session->dynamic_elements = g_list_append(data->session->dynamic_elements, priv->video_bin);
    gst_element_sync_state_with_parent(priv->video_bin);

exit:
    data->done = 1;
    C_TRACE("%s GstRtspPlayerSession__idle_attach_video_pad - done", data->session->location);
    P_COND_BROADCAST(data->cond);
    return FALSE;
}

static void
GstRtspPlayerSession__on_rtsp_pad_added (GstElement *element, GstPad *new_pad, GstRtspPlayerSession * session){
    C_DEBUG ("%s Received new pad '%s' from '%s'", session->location, GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (element));
    
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
        /*
            gtkglsink requires to be attached on the main thread
            pad_added is called by the streaming thread, so we sync with the main thread to attach it
        */
        struct create_video_data data;
        data.session = session;
        data.new_pad = new_pad;
        data.element = element;
        data.done = 0;
        P_COND_SETUP(data.cond);
        P_MUTEX_SETUP(data.lock);

        C_TRACE("%s dispatch idle_attach_video_pad", session->location);
        // g_idle_add(G_SOURCE_FUNC(GstRtspPlayerSession__idle_attach_video_pad),&data);
        GstRtspPlayerSession__idle_attach_video_pad(&data);

        if(!data.done) { C_TRACE("%s wait idle_attach_video_pad", session->location); P_COND_WAIT(data.cond, data.lock); }
        P_COND_CLEANUP(data.cond);
        P_MUTEX_CLEANUP(data.lock);

    } else if (g_strrstr(capsName,"audio")){
        GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);
        gst_bin_add_many (GST_BIN (session->pipeline), priv->audio_bin, NULL);

        sink_pad = gst_element_get_static_pad (priv->audio_bin, "bin_sink");
        pad_ret = gst_pad_link (new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED (pad_ret)) {
            C_ERROR ("%s failed to link dynamically '%s' to '%s'", session->location,GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(priv->audio_bin));
            goto exit;
        }
        session->dynamic_elements = g_list_append(session->dynamic_elements, priv->audio_bin);
        gst_element_sync_state_with_parent(priv->audio_bin);
    } else {
        new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
        gint payload_v;
        gst_structure_get_int(new_pad_struct,"payload", &payload_v);
        C_ERROR("%s Support other payload formats %i", session->location,payload_v);
    }

exit:
    C_DEBUG ("%s Received new pad attached", session->location);
    free(capsName);
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps)
        gst_caps_unref (new_pad_caps);
    if (sink_pad)
        gst_object_unref (sink_pad);
}

static GstRtspPlayerSession *
GstRtspPlayerSession__setup_pipeline (GstRtspPlayerSession * session)
{

    /* Create the empty pipeline */
    session->pipeline = gst_pipeline_new ("onvif-pipeline");

    /* Create the elements */
    session->src = gst_element_factory_make ("rtspsrc", "rtspsrc");

    if (!session->pipeline){
        C_FATAL("%s Failed to created pipeline. Check your gstreamer installation...", session->location);
        return NULL;
    }
    if (!session->src){
        C_FATAL ("%s Failed to created rtspsrc. Check your gstreamer installation...", session->location);
        return NULL;
    }

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (session->pipeline), session->src, NULL);

    // Dynamic Pad Creation
    if(! g_signal_connect (session->src, "pad-added", G_CALLBACK (GstRtspPlayerSession__on_rtsp_pad_added),session)){
        C_ERROR ("%s Linking part (1) with part (A)-1 Fail...", session->location);
    }

    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);
    if(!g_signal_connect (session->src, "select-stream", G_CALLBACK (RtspBackchannel__find),priv->backchannel)){
        C_ERROR ("%s Fail to connect select-stream signal...", session->location);
    }

    // g_object_set (G_OBJECT (priv->src), "buffer-mode", 3, NULL);
    g_object_set (G_OBJECT (session->src), "latency", 0, NULL);
    g_object_set (G_OBJECT (session->src), "teardown-timeout", 0, NULL); 
    g_object_set (G_OBJECT (session->src), "backchannel", session->enable_backchannel, NULL);
    g_object_set (G_OBJECT (session->src), "user-agent", "OnvifDeviceManager-Linux-0.0", NULL);
    g_object_set (G_OBJECT (session->src), "do-retransmission", TRUE, NULL);
    g_object_set (G_OBJECT (session->src), "onvif-mode", FALSE, NULL); //It seems onvif mode can cause segmentation fault with libva
    g_object_set (G_OBJECT (session->src), "is-live", TRUE, NULL);
    g_object_set (G_OBJECT (session->src), "tcp-timeout", 10000, NULL);
    g_object_set (G_OBJECT (session->src), "protocols", GST_RTSP_LOWER_TRANS_TCP, NULL); //TODO Allow changing this via settings

    /* set up bus */
    GstBus *bus = gst_element_get_bus (session->pipeline);
    GSource *source = gst_bus_create_watch (bus);
    if (!source) {
        g_critical ("Creating bus watch failed");
        return 0;
    }
    // g_source_set_priority (source, priority);
    g_source_set_callback (source, G_SOURCE_FUNC(GstRtspPlayerSession__message_handler), session, NULL);
    if(priv->player_context){
        g_source_attach (source, priv->player_context);
    } else {
        GMainContext *ctx = g_main_context_get_thread_default ();
        g_source_attach (source, ctx);
    }

    g_source_unref (source);
    gst_object_unref (bus);

    return session;
}

gboolean GstRtspPlayerPrivate__idle_hide(GtkWidget * widget){
    gtk_widget_hide(widget);
    return FALSE;
}

gboolean GstRtspPlayerPrivate__idle_show(GtkWidget * widget){
    gtk_widget_show(widget);
    return FALSE;
}

gboolean GstRtspPlayerPrivate__stop_unlocked(GstRtspPlayerPrivate * priv){
    GstStateChangeReturn ret;

    if(!priv->session){
        C_TRACE("Nothing to stop.");
        return FALSE;
    }

    //New pipeline causes previous pipe to stop dispatching state change.
    //Force hide the previous stream
    g_main_context_invoke(g_main_context_default(),G_SOURCE_FUNC(GstRtspPlayerPrivate__idle_hide),priv->canvas);

    //Pause backchannel
    if(!RtspBackchannel__pause(priv->backchannel)){
        C_WARN("Failed to pause backchannel.");
    }

    //Set NULL state to clear buffers
    if(GST_IS_ELEMENT(priv->session->pipeline)){
        ret = gst_element_set_state (priv->session->pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            C_ERROR ("%s Unable to set the pipeline to the ready state.",priv->session->location);
        }
    }

    //Destroy old pipeline
    if(GST_IS_ELEMENT(priv->session->pipeline)){
        gst_object_unref (priv->session->pipeline);
        priv->session->pipeline = NULL;
    }

    if(priv->session->dynamic_elements){
        g_list_free(priv->session->dynamic_elements);
        priv->session->dynamic_elements = NULL;
    }
    return TRUE;
}

void GstRtspPlayerPrivate__stop(GstRtspPlayerPrivate * priv){
    C_DEBUG("GstRtspPlayerPrivate__stop\n");
    P_MUTEX_LOCK(priv->player_lock);
    priv->playing = 0;

    if(GstRtspPlayerPrivate__stop_unlocked(priv))
        player_signal_and_wait (priv->owner, signals[STOPPED]);
    
    P_MUTEX_UNLOCK(priv->player_lock);
}


void GstRtspPlayer__stop(GstRtspPlayer* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    GstRtspPlayerPrivate__stop(priv);
}

void GstRtspPlayerSession__play(GstRtspPlayerSession * session){
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);
    P_MUTEX_LOCK(priv->player_lock);
    GstRtspPlayerPrivate__stop_unlocked(priv);
    if(priv->session != session){
        GstRtspPlayerSession__destroy(priv->session);
        priv->session = session;
    }
    GstRtspPlayerSession__setup_pipeline(session);

    if(session->user)
        g_object_set (G_OBJECT (session->src), "user-id", session->user, NULL);
    if(session->pass)
        g_object_set (G_OBJECT (session->src), "user-pw", session->pass, NULL);
    if(session->location)
        g_object_set (G_OBJECT (session->src), "location", session->location, NULL);

    C_DEBUG("%s RtspPlayer__play retry[%i] - playing[%i]",session->location,session->retry,priv->playing);
    priv->playing = 1;

    GstStateChangeReturn ret;
    ret = gst_element_set_state (session->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        C_ERROR ("%s Unable to set the pipeline to the playing state.",session->location);
        gst_object_unref (session->pipeline);
        session->pipeline = NULL;
    }
    P_MUTEX_UNLOCK(priv->player_lock);
}

void GstRtspPlayer__play(GstRtspPlayer* self, char *url, char * user, char * pass, char * fallback_host, char * fallback_port, void * user_data){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));

    GstRtspPlayerSession * session = GstRtspPlayerSession__create(self, url, user, pass, fallback_host, fallback_port, user_data);
    GstRtspPlayerSession__play(session);
}

/*
Compared to play, retry is design to work after a stream failure.
Stopping will essentially break the retry method and stop the loop.
*/
void GstRtspPlayerSession__retry(GstRtspPlayerSession * session){
    //TODO Typecheck
    GstRtspPlayerSession__play(session);
}

static void 
GstRtspPlayerSession__warning_msg (GstRtspPlayerSession * session, GstBus *bus, GstMessage *msg) {
    GError *err;
    gchar *debug_info;
    gst_message_parse_warning (msg, &err, &debug_info);
    if(strcmp(err->message,"Empty Payload.")){ //Ignoring rtp empty payload warnings
        C_WARN ("%s Warning received from element %s: %s",session->location, GST_OBJECT_NAME (msg->src), err->message);
        C_WARN ("%s Debugging information: %s",session->location, debug_info ? debug_info : "none");
    }
    g_clear_error (&err);
    g_free (debug_info);
}

static int GstRtspPlayerSession__process_fallback(GstRtspPlayerSession * session){
    switch(session->fallback){
        case RTSP_FALLBACK_NONE:
            session->fallback = RTSP_FALLBACK_HOST;
            char * host_fallback = NULL;
            if(session->host_fallback) host_fallback = URL__set_host(session->location_set, session->host_fallback);
            if(host_fallback && strcmp(host_fallback,session->location_set) != 0 && strcmp(host_fallback,session->location) != 0){
                C_WARN("%s Attempting URL host correction to [%s]",session->location_set, host_fallback);
                free(session->location);
                session->location = host_fallback;
                break;
            } else {
                free(host_fallback);
            }
            __attribute__ ((fallthrough));
        case RTSP_FALLBACK_HOST:
            session->fallback = RTSP_FALLBACK_PORT;
            char * port_fallback = NULL;
            if(session->port_fallback) port_fallback = URL__set_port(session->location_set, session->port_fallback);
            if(port_fallback && strcmp(port_fallback,session->location_set) != 0 && strcmp(port_fallback,session->location) != 0){
                C_WARN("%s Attempting URL port correction to [%s]",session->location_set, port_fallback);
                free(session->location);
                session->location = port_fallback;
                break;
            } else {
                free(port_fallback);
            }
            __attribute__ ((fallthrough));
        case RTSP_FALLBACK_PORT:
            session->fallback = RTSP_FALLBACK_URL;
            char * tmp_fallback = NULL;
            char * url_fallback = NULL;
            if(session->host_fallback && session->port_fallback){
                tmp_fallback = URL__set_port(session->location_set, session->port_fallback);
                url_fallback = URL__set_host(tmp_fallback, session->host_fallback);
            }
            if(url_fallback && strcmp(url_fallback,session->location_set) != 0 && strcmp(url_fallback,session->location) != 0){
                C_WARN("%s Attempting root URL correction to [%s]",session->location_set, url_fallback);
                free(session->location);
                session->location = url_fallback;
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
            // priv->playing = 0;
            // C_ERROR ("No more fallback option for %s", priv->location_set);
            // g_signal_emit (priv->owner, signals[ERROR], 0 /* details */);
            return 0;
    }
    return 1;
}

/* This function is called when an error message is posted on the bus */
static void 
GstRtspPlayerSession__error_msg (GstRtspPlayerSession * session, GstBus *bus, GstMessage *msg) {
    GError *err;
    gchar *debug_info;
    
    int fallback = 0;
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);
    P_MUTEX_LOCK(priv->player_lock);
    gst_message_parse_error (msg, &err, &debug_info);

    switch(err->code){
        case GST_RESOURCE_ERROR_SETTINGS:
            C_WARN ("%s Backchannel unsupported. Downgrading...", session->location);
            if(session->enable_backchannel){
                session->enable_backchannel = 0;
                session->retry--; //This doesn't count as a try. Finding out device capabilities count has handshake
            } else {
                C_ERROR ("%s Error received from element %s: %s", session->location, GST_OBJECT_NAME (msg->src), err->message);
                C_ERROR ("%s Debugging information: %s", session->location, debug_info ? debug_info : "none");
                C_ERROR ("%s Error code : %d", session->location,err->code);
            }
            break;
        case GST_RESOURCE_ERROR_READ:
            if(!session->valid_location && !strcmp(err->message,"Unhandled error")){
                //Most likely invalid handshake response, like HTTP 400 - allow fallthrough into fallback logic
                // __attribute__ ((fallthrough));
            } else if(!strcmp(err->message,"Could not read from resource.")){
                // We allow these errors to retry without fallback with less debug
                // This may happen with unreliable network route
                C_ERROR ("%s Error received from element [%d] %s: %s", session->location, err->code, GST_OBJECT_NAME (msg->src), err->message);
                break;
            } else {
                // We allow other errors to retry without fallback with added debug
                C_ERROR ("%s Error received from element [%d] %s: %s", session->location, err->code, GST_OBJECT_NAME (msg->src), err->message);
                C_ERROR ("%s Debugging information: %s", session->location, debug_info ? debug_info : "none");
                break;
            }
        case GST_RESOURCE_ERROR_OPEN_READ:
        case GST_RESOURCE_ERROR_OPEN_WRITE:
        case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
            /*
                Fallback may be necessary when the camera is behind a load balancer changin the front facing IP/Port of the device
                The camera will return its own address unaware of the loadbalancer.
            */ 
            C_ERROR ("%s Failed to connect", session->location);
            if(session->retry >=3 && !session->valid_location && (session->port_fallback || session->host_fallback)){
                fallback = GstRtspPlayerSession__process_fallback(session);
                if(fallback){
                    session->retry = 0;
                }
            }
            break;
        case GST_RESOURCE_ERROR_NOT_AUTHORIZED:
        case GST_RESOURCE_ERROR_NOT_FOUND:
            C_ERROR("%s Non-recoverable error encountered.", session->location);
            priv->playing = 0;
            __attribute__ ((fallthrough));
        default:
            C_ERROR ("%s Error received from element [%d] %s: %s",session->location,err->code, GST_OBJECT_NAME (msg->src), err->message);
            C_ERROR ("%s Debugging information: %s",session->location, debug_info ? debug_info : "none");
            player_signal_and_wait (priv->owner, signals[ERROR], session);
    }

    g_clear_error (&err);
    g_free (debug_info);

    if((fallback && priv->playing) || (session->retry < 3 && priv->playing)){ //TODO Check if any retry callback exists
        //Stopping player after if condition because "playing" gets reset
        GstRtspPlayerPrivate__stop_unlocked(priv);
        C_WARN("****************************************************");
        if(fallback) C_WARN("* URI fallback attempt %s", session->location); else C_WARN("* Retry attempt #%i - %s",session->retry, session->location);
        C_WARN("****************************************************");
        //Retry signal - The player doesn't invoke retry on its own to allow the invoker to dispatch it asynchroniously
        if(!fallback)
            session->retry++;
        P_MUTEX_UNLOCK(priv->player_lock);
        player_signal_and_wait (priv->owner, signals[RETRY], session);
        return;
    } else if(priv->playing == 1) {
        C_TRACE("%s Player giving up. Too many retries...", session->location);
        priv->playing = 0;
        GstRtspPlayerPrivate__stop_unlocked(priv);
        P_MUTEX_UNLOCK(priv->player_lock);
        //Error signal
        player_signal_and_wait (priv->owner, signals[ERROR], session);
        return;
    } else { //Ignoring error after the player requested to stop (gst_rtspsrc_try_send)
        C_TRACE("Player no longer playing...");
    }
    P_MUTEX_UNLOCK(priv->player_lock);
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void 
GstRtspPlayerSession__eos_msg (GstRtspPlayerSession * session, GstBus *bus, GstMessage *msg) {
    C_ERROR ("%s End-Of-Stream reached.\n", session->location);
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);
    GstRtspPlayerPrivate__stop(priv);
}

int GstRtspPlayerSession__is_dynamic_pad(GstRtspPlayerSession * session, GstElement * element){
    if(g_list_length(session->dynamic_elements) == 0){
        return FALSE;
    }
    GList *node_itr = session->dynamic_elements;
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
GstRtspPlayerSession__is_video_bin(GstElement * element){
    if(GST_IS_OBJECT(element) && strcmp(GST_OBJECT_NAME(element),"video_bin") == 0){
        return TRUE;
    }

    return FALSE;
}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void
GstRtspPlayerSession__state_changed_msg (GstRtspPlayerSession * session, GstBus * bus, GstMessage * msg)
{
    GstState old_state, new_state, pending_state;
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);

    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    /* Using video_bin for state change since the pipeline is PLAYING before the videosink */
    
    GstElement * element = GST_ELEMENT(GST_MESSAGE_SRC (msg));

    if(GstRtspPlayerSession__is_dynamic_pad(session,element)){
        C_TRACE ("%s State set to %s for %s\n", session->location, gst_element_state_get_name (new_state), GST_OBJECT_NAME (msg->src));
    }

    if(GstRtspPlayerSession__is_video_bin(element) && new_state != GST_STATE_PLAYING && GTK_IS_WIDGET (priv->canvas)){
        g_main_context_invoke(g_main_context_default(),G_SOURCE_FUNC(GstRtspPlayerPrivate__idle_hide),priv->canvas);
    } else if(GstRtspPlayerSession__is_video_bin(element) && new_state == GST_STATE_PLAYING && GTK_IS_WIDGET (priv->canvas)){
        g_main_context_invoke(g_main_context_default(),G_SOURCE_FUNC(GstRtspPlayerPrivate__idle_show),priv->canvas);

        /*
        * Waiting for fix https://gitlab.freedesktop.org/gstreamer/gst-plugins-good/-/issues/245
        * Until This issue is fixed, "no-more-pads" is unreliable to determine if all stream states are ready.
        * Until then, we only rely on the video stream state to fire the started signal
        * Funny enough, a gstreamer generated onvif rtsp stream causes this issue
        * 
        * "select-stream" might be an alternative, although I don't know if the first stream could play before the last one is shown
        */
        session->retry = 0;
        session->valid_location = 1;
        session->fallback = RTSP_FALLBACK_NONE;

        player_signal_and_wait (priv->owner, signals[STARTED],session);
    }
}


static gboolean 
GstRtspPlayerSession__message_handler (GstBus * bus, GstMessage * message, GstRtspPlayerSession * session)
{ 
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (session->player);
    const GstStructure *s;
    const gchar *name;
    switch(GST_MESSAGE_TYPE(message)){
        case GST_MESSAGE_UNKNOWN:
            C_TRACE("%s msg : GST_MESSAGE_UNKNOWN\n", session->location);
            break;
        case GST_MESSAGE_EOS:
            GstRtspPlayerSession__eos_msg(session,bus,message);
            break;
        case GST_MESSAGE_ERROR:
            GstRtspPlayerSession__error_msg(session,bus,message);
            break;
        case GST_MESSAGE_WARNING:
            GstRtspPlayerSession__warning_msg(session,bus,message);
            break;
        case GST_MESSAGE_INFO:
            C_TRACE("%s msg : GST_MESSAGE_INFO", session->location);
            break;
        case GST_MESSAGE_TAG:
            // tag_cb(bus,message,player);
            break;
        case GST_MESSAGE_BUFFERING:
            C_TRACE("%s msg : GST_MESSAGE_BUFFERING", session->location);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            GstRtspPlayerSession__state_changed_msg(session, bus,message);
            break;
        case GST_MESSAGE_STATE_DIRTY:
            C_TRACE("%s msg : GST_MESSAGE_STATE_DIRTY", session->location);
            break;
        case GST_MESSAGE_STEP_DONE:
            C_TRACE("%s msg : GST_MESSAGE_STEP_DONE", session->location);
            break;
        case GST_MESSAGE_CLOCK_PROVIDE:
            C_TRACE("%s msg : GST_MESSAGE_CLOCK_PROVIDE", session->location);
            break;
        case GST_MESSAGE_CLOCK_LOST:
            C_TRACE("%s msg : GST_MESSAGE_CLOCK_LOST", session->location);
            break;
        case GST_MESSAGE_NEW_CLOCK:
            break;
        case GST_MESSAGE_STRUCTURE_CHANGE:
            C_TRACE("%s msg : GST_MESSAGE_STRUCTURE_CHANGE", session->location);
            break;
        case GST_MESSAGE_STREAM_STATUS:
            break;
        case GST_MESSAGE_APPLICATION:
            C_TRACE("%s msg : GST_MESSAGE_APPLICATION", session->location);
            break;
        case GST_MESSAGE_ELEMENT:
            s = gst_message_get_structure (message);
            name = gst_structure_get_name (s);
            if (strcmp (name, "level") == 0) {
                OverlayState__level_handler(bus,message,priv->overlay_state,s);
            } else if (strcmp (name, "GstNavigationMessage") == 0 || 
                        strcmp (name, "application/x-rtp-source-sdes") == 0){
                //Ignore intentionally left unhandled for now
            } else if (strcmp (name, "GstRTSPSrcTimeout") == 0){
                C_WARN("RtspServer rtp stream timedout [GstRTSPSrcTimeout]");
                GstRtspPlayerPrivate__stop(priv);
                session->retry=0;
                player_signal_and_wait (priv->owner, signals[RETRY], session);
            } else {
                C_ERROR("%s Unhandled element msg name : %s//%d", session->location,name,message->type);
            }
            break;
        case GST_MESSAGE_SEGMENT_START:
            C_TRACE("%s msg : GST_MESSAGE_SEGMENT_START", session->location);
            break;
        case GST_MESSAGE_SEGMENT_DONE:
            C_TRACE("%s msg : GST_MESSAGE_SEGMENT_DONE", session->location);
            break;
        case GST_MESSAGE_DURATION_CHANGED:
            C_TRACE("%s msg : GST_MESSAGE_DURATION_CHANGED", session->location);
            break;
        case GST_MESSAGE_LATENCY:
            break;
        case GST_MESSAGE_ASYNC_START:
            C_TRACE("%s msg : GST_MESSAGE_ASYNC_START", session->location);
            break;
        case GST_MESSAGE_ASYNC_DONE:
            // printf("msg : GST_MESSAGE_ASYNC_DONE");
            break;
        case GST_MESSAGE_REQUEST_STATE:
            C_TRACE("%s msg : GST_MESSAGE_REQUEST_STATE", session->location);
            break;
        case GST_MESSAGE_STEP_START:
            C_TRACE("%s msg : GST_MESSAGE_STEP_START", session->location);
            break;
        case GST_MESSAGE_QOS:
            break;
        case GST_MESSAGE_PROGRESS:
            break;
        case GST_MESSAGE_TOC:
            C_TRACE("%s msg : GST_MESSAGE_TOC", session->location);
            break;
        case GST_MESSAGE_RESET_TIME:
            C_TRACE("%s msg : GST_MESSAGE_RESET_TIME", session->location);
            break;
        case GST_MESSAGE_STREAM_START:
            break;
        case GST_MESSAGE_NEED_CONTEXT:
            break;
        case GST_MESSAGE_HAVE_CONTEXT:
            break;
        case GST_MESSAGE_EXTENDED:
            C_TRACE("%s msg : GST_MESSAGE_EXTENDED", session->location);
            break;
        case GST_MESSAGE_DEVICE_ADDED:
            C_TRACE("%s msg : GST_MESSAGE_DEVICE_ADDED", session->location);
            break;
        case GST_MESSAGE_DEVICE_REMOVED:
            C_TRACE("%s msg : GST_MESSAGE_DEVICE_REMOVED", session->location);
            break;
        case GST_MESSAGE_PROPERTY_NOTIFY:
            C_TRACE("%s msg : GST_MESSAGE_PROPERTY_NOTIFY", session->location);
            break;
        case GST_MESSAGE_STREAM_COLLECTION:
            C_TRACE("%s msg : GST_MESSAGE_STREAM_COLLECTION", session->location);
            break;
        case GST_MESSAGE_STREAMS_SELECTED:
            C_TRACE("%s msg : GST_MESSAGE_STREAMS_SELECTED", session->location);
            break;
        case GST_MESSAGE_REDIRECT:
            C_TRACE("%s msg : GST_MESSAGE_REDIRECT", session->location);
            break;
        //Doesnt exists in gstreamer 1.16
        // case GST_MESSAGE_DEVICE_CHANGED:
        //  printf("msg : GST_MESSAGE_DEVICE_CHANGED\n");
        //  break;
        // case GST_MESSAGE_INSTANT_RATE_REQUEST:
        //  printf("msg : GST_MESSAGE_INSTANT_RATE_REQUEST\n");
        //  break;
        case GST_MESSAGE_ANY:
            C_WARN("%s msg : GST_MESSAGE_ANY", session->location);
            break;
        default:
            C_WARN("%s msg : default....", session->location);
    }
    return TRUE;
}

void * init_gst_seprate_mainloop(void * event){
    struct GstInitData * data = (struct GstInitData*)event;
    //Keeping location pointer because data won't be value after cond broadcast
    GMainLoop ** loop = data->loop;
    P_COND_TYPE * finish_cond = data->finish_cond;

    c_log_set_thread_color(ANSI_COLOR_CYAN, P_THREAD_ID);
    *data->context = g_main_context_new ();
    *loop = g_main_loop_new (*data->context, FALSE);

    data->done = 1;          //Flag that the context and loop are ready
    P_COND_BROADCAST(data->cond);
    g_main_loop_run (*loop);
    *loop = NULL;            //Setting loop to NULL flags it as finished
    P_COND_BROADCAST(*finish_cond); //broadcast to potentially waiting threads

    return NULL;
}

static void
GstRtspPlayer__init (GstRtspPlayer * self)
{
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    priv->owner = self;
    P_COND_SETUP(priv->loop_cond);
    P_MUTEX_SETUP(priv->loop_lock);

    struct GstInitData data;
    data.done = 0;
    data.finish_cond = &priv->loop_cond;
    data.context = &priv->player_context;
    data.loop = &priv->player_loop;
    P_COND_SETUP(data.cond);
    P_MUTEX_SETUP(data.lock);

    pthread_t pthread;
    pthread_create(&pthread, NULL, init_gst_seprate_mainloop, &data);
    pthread_detach(pthread);

    if(!data.done) { C_TRACE("Waiting for Gstreamer loop initialization"); P_COND_WAIT(data.cond, data.lock); }
    P_COND_CLEANUP(data.cond);
    P_MUTEX_CLEANUP(data.lock);
    C_TRACE("Gstreamer loop initialization successfull");

    priv->view_mode = GST_RTSP_PLAYER_VIEW_MODE_FIT_WINDOW;
    priv->overlay_state = OverlayState__create();
    priv->canvas_handle = gtk_grid_new ();
    priv->canvas = NULL;
    priv->sink = NULL;
    priv->snapsink = NULL;
    priv->playing = 0;
    priv->sinkcaps = NULL;
    priv->video_bin = GstRtspPlayerPrivate__create_video_pad(priv);
    g_object_ref(priv->video_bin);
    priv->audio_bin = GstRtspPlayerPrivate__create_audio_pad();
    g_object_ref(priv->audio_bin);

    priv->backchannel = RtspBackchannel__create(priv->player_context);
}

GstRtspPlayer*  GstRtspPlayer__new (){
    GstRtspPlayer * player = g_object_new (GST_TYPE_RTSPPLAYER, NULL);
    return player;
}

GtkWidget * GstRtspPlayer__createCanvas(GstRtspPlayer *self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (GST_IS_RTSPPLAYER (self), NULL);
    
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);

    GtkWidget * scroll = gtk_scrolled_window_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(scroll),priv->canvas_handle);
    return scroll;
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

void GstRtspPlayer__set_view_mode(GstRtspPlayer * self, GstRtspViewMode mode){
    g_return_if_fail (self != NULL);
    g_return_if_fail (GST_IS_RTSPPLAYER (self));

    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);
    priv->view_mode = mode;

    if(GTK_IS_WIDGET(priv->canvas)){
        GstRtspPlayerPrivate__apply_view_mode(priv);
    }
}

GstSnapshot * GstRtspPlayer__get_snapshot(GstRtspPlayer* self){
    g_return_val_if_fail (self != NULL,NULL);
    g_return_val_if_fail (GST_IS_RTSPPLAYER (self),NULL);

    GstSnapshot * snap = NULL;
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (self);

    if(!priv->session){
        C_TRACE("Nothing to snapshot");
        goto exit;
    }
    C_TRACE("%s GstRtspPlayer__get_snapshot",priv->session->location);
    P_MUTEX_LOCK(priv->player_lock);
    GstState state = GST_STATE_NULL;
    GstStateChangeReturn ret_1 = gst_element_get_state (priv->sink,&state, NULL, GST_CLOCK_TIME_NONE);
    if(ret_1 == GST_STATE_CHANGE_SUCCESS && state == GST_STATE_PLAYING){
        GstSample * last_sample = gst_base_sink_get_last_sample(GST_BASE_SINK_CAST(priv->snapsink));
        last_sample = gst_base_sink_get_last_sample(GST_BASE_SINK_CAST(priv->snapsink));
        GstSample * png_sample = NULL;

        if(last_sample){
            C_WARN("%s Extracted sample.",priv->session->location);
            GError * error = NULL;
            GstCaps * out_caps = gst_caps_from_string ("image/jpeg,width=640,height=480");
            png_sample = gst_video_convert_sample(last_sample, out_caps, GST_CLOCK_TIME_NONE, &error);
            if(error){
                if(error->message){
                    C_ERROR("%s GstRtspPlayer encountered an error extracting snapshot. [%d] %s",priv->session->location , error->code, error->message);
                } else {
                    C_ERROR("%s GstRtspPlayer encountered an error extracting snapshot. [%d]",priv->session->location , error->code);
                }
            }
        }

        if(png_sample){
            GstBuffer * buffer = gst_sample_get_buffer(png_sample);
            GstMapInfo map_info;
            memset (&map_info, 0, sizeof (map_info));

            if (gst_buffer_map (buffer, &map_info, GST_MAP_READ)) {
                snap = malloc(sizeof(GstSnapshot));
                snap->size = map_info.size;
                snap->data = malloc(snap->size);
                memcpy (snap->data, map_info.data, snap->size);
            } else {
                C_ERROR("%s GstRtspPlayer Failed to extract snapshot sample data",priv->session->location);
            }
        }

        if(last_sample)
            gst_sample_unref(last_sample);
        if(png_sample)
            gst_sample_unref(png_sample);
    } else {
        C_ERROR("%s GstRtspPlayer not playing. No snapshot extracted %d - %d",priv->session->location,ret_1,state);
    }
    P_MUTEX_UNLOCK(priv->player_lock);
exit:
    return snap;
}

void GstSnapshot__destroy(GstSnapshot * snapshot){
    if(!snapshot) return;

    free(snapshot->data);
    free(snapshot);   
}

static void
GstRtspPlayer__dispose (GObject *gobject)
{
    GstRtspPlayerPrivate *priv = GstRtspPlayer__get_instance_private (GST_RTSPPLAYER (gobject));

    //Making sure stream is stopped
    GstRtspPlayerPrivate__stop(priv);
    if(priv->player_loop){
        g_main_loop_quit(priv->player_loop);
        //Making sure the loop is finished so that session isn't destroyed while an event is running
        if(priv->player_loop != NULL) { C_TRACE("Waiting for Gstreamer loop to finish"); P_COND_WAIT(priv->loop_cond, priv->loop_lock); }
    }

    P_COND_CLEANUP(priv->loop_cond);
    P_MUTEX_CLEANUP(priv->loop_lock);

    if(priv->player_context){
        g_main_context_unref(priv->player_context);
        priv->player_context = NULL;
    }

    if(priv->session){
        GstRtspPlayerSession__destroy(priv->session);
        priv->session = NULL;
    }
    OverlayState__destroy(priv->overlay_state);
    RtspBackchannel__destroy(priv->backchannel);
    
    P_MUTEX_CLEANUP(priv->player_lock);
    if(priv->video_bin){
        g_object_unref(priv->video_bin);
        priv->video_bin = NULL;
    }

    if(priv->audio_bin){
        g_object_unref(priv->audio_bin);
        priv->audio_bin = NULL;
    }

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

    GType params[1];
    params[0] = G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[STARTED] =
        g_signal_newv ("started",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                1     /* n_params */,
                params  /* param_types */);

    signals[RETRY] =
        g_signal_newv ("retry",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                1     /* n_params */,
                params  /* param_types */);

    signals[ERROR] =
        g_signal_newv ("error",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                NULL /* closure */,
                NULL /* accumulator */,
                NULL /* accumulator data */,
                NULL /* C marshaller */,
                G_TYPE_NONE /* return_type */,
                1     /* n_params */,
                params  /* param_types */);

}
