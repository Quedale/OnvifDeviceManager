#include "backchannel.h"
#include "portable_thread.h"
#include <stdio.h>
#include "src_retriever.h"
#include "clogger.h"

static P_MUTEX_TYPE init_lock = P_MUTEX_INITIALIZER;
static int initialized = 0;
static char mic_element[13], mic_device[8];

typedef struct _RtspBackchannel {
    GstElement * pipeline;
    GstElement *mic_volume_element;
    GstElement *appsink;
    int back_stream_id;
    GstElement * rtspsrc;
} RtspBackchannel;

static void RtspBackchannel__message_handler (GstBus * bus, GstMessage * message, gpointer p);
GstFlowReturn RtspBackchannel__new_sample (GstElement * appsink, RtspBackchannel * self);
gboolean RtspBackchannel__remove_extra_fields (GQuark field_id, GValue * value G_GNUC_UNUSED, gpointer user_data G_GNUC_UNUSED);

RtspBackchannel * RtspBackchannel__create(){
    RtspBackchannel * self = malloc(sizeof(RtspBackchannel));
    RtspBackchannel__init(self);
    return self;
}

void RtspBackchannel__destroy(RtspBackchannel * self){
    if(self){
        if(GST_IS_ELEMENT(self->pipeline)){
            gst_object_unref (self->pipeline);
        }
        free(self);
    }
}

GstStateChangeReturn RtspBackchannel__pause(RtspBackchannel * self){
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS ;
    if(self && GST_IS_ELEMENT(self->pipeline)){
      ret = gst_element_set_state (self->pipeline, GST_STATE_NULL);
      if (ret == GST_STATE_CHANGE_FAILURE) {
            C_ERROR ("Unable to set the pipeline to ready state.");
      }
    }
    return ret;
}

GstStateChangeReturn RtspBackchannel__stop(RtspBackchannel * self){
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    if(self && GST_IS_ELEMENT(self->pipeline)){
        ret = gst_element_set_state (self->pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            C_ERROR ("Unable to set the pipeline to null state.");
        }
    }
    return ret;
}

gboolean RtspBackchannel__is_mute(RtspBackchannel* self){
    if(self && GST_IS_ELEMENT(self->mic_volume_element)){
        gboolean v;
        g_object_get (G_OBJECT (self->mic_volume_element), "mute", &v, NULL);
        return v;
    } else {
        return TRUE;
    }
}

void RtspBackchannel__mute(RtspBackchannel* self, gboolean mute) {
    if(self && GST_IS_ELEMENT(self->mic_volume_element)){
        g_object_set (G_OBJECT (self->mic_volume_element), "mute", mute, NULL);
    }
}

GstStateChangeReturn RtspBackchannel__get_state(RtspBackchannel * self, GstState * state, GstState * nstate){
    if(self && GST_IS_ELEMENT(self->pipeline)){
        return gst_element_get_state (self->pipeline,state, nstate, GST_CLOCK_TIME_NONE);
    }
    return GST_STATE_CHANGE_FAILURE;
}

void RtspBackchannel__check_mic(RtspBackchannel * self){
    P_MUTEX_LOCK(init_lock);

    if(initialized) return;

    initialized = 1;
    retrieve_audiosrc(mic_element,mic_device);

    P_MUTEX_UNLOCK(init_lock);
}

void RtspBackchannel__init(RtspBackchannel * self){
    GstElement *src;
    GstElement *enc;
    GstElement *pay;

    self->rtspsrc = NULL;
    RtspBackchannel__check_mic(self);

    //self->mic_element
    self->pipeline = gst_pipeline_new ("backchannel-pipeline");

    /* Create the elements */
    if(strlen(mic_element) == 0){
        C_WARN("No microphone detected. Skipping backchannel setup!");
        return;
    }

    C_INFO("Creating backchannel using source element %s",mic_element);

    src = gst_element_factory_make (mic_element, NULL);
    self->mic_volume_element = gst_element_factory_make ("volume", NULL);
    enc = gst_element_factory_make ("mulawenc", NULL);
    pay = gst_element_factory_make ("rtppcmupay", NULL);
    self->appsink = gst_element_factory_make ("appsink", NULL);

    if(!src || !self->mic_volume_element || !enc || !pay || !self->appsink){
        C_FATAL("Failed to created backchannel element(s). Check your gstreamer installation...");
        return;
    }

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (self->pipeline), src, self->mic_volume_element, enc, pay, self->appsink, NULL);

    // Link confirmation
    if (!gst_element_link_many (src,
        self->mic_volume_element,
        enc, pay, self->appsink, NULL)){
        C_FATAL ("Failed to link pipeline element...");
        return;
    }

    if(!strcmp(mic_element,"alsasrc") 
        && strlen(mic_device) > 1){
        C_INFO("Setting alsa mic to '%s'\n",mic_device);
        g_object_set(G_OBJECT(src), "device", mic_device,NULL);
    }

    // g_object_set (G_OBJECT (src), "sync", FALSE, NULL);
    g_object_set (G_OBJECT (self->mic_volume_element), "mute", TRUE, NULL);
    g_object_set (G_OBJECT (self->appsink), "emit-signals", TRUE, NULL);
    if(!g_signal_connect (self->appsink, "new-sample", G_CALLBACK (RtspBackchannel__new_sample), self)){
        C_FATAL("Failed to add new-sample signal to appsink. Check your gstreamer installation...\n");
        return;
    }

    /* set up pipeline bus */
    GstBus *bus = gst_element_get_bus (self->pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (RtspBackchannel__message_handler), self);
    gst_object_unref (bus);
}

gboolean RtspBackchannel__find (GstElement * rtspsrc, guint idx, GstCaps * caps, RtspBackchannel *self G_GNUC_UNUSED){
    GstStructure *s;
    
    self->rtspsrc = rtspsrc;

    if(!self->pipeline){
        C_ERROR("Backchannel not setup\n");
        goto exit;
    }

    gchar *caps_str = gst_caps_to_string (caps);
    g_free (caps_str);
    s = gst_caps_get_structure (caps, 0);
    if (gst_structure_has_field (s, "a-sendonly")) {
        self->back_stream_id = idx;
        caps = gst_caps_new_empty ();
        s = gst_structure_copy (s);
        gst_structure_set_name (s, "application/x-rtp");
        gst_structure_filter_and_map_in_place (s, RtspBackchannel__remove_extra_fields, NULL);
        gst_caps_append_structure (caps, s);

        //Update appsink caps compatible with recipient
        g_object_set (G_OBJECT (self->appsink), "caps", caps, NULL);
        C_DEBUG ("Playing backchannel shoveler\n");
        gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
        gst_caps_unref (caps);
    }

exit:
    return TRUE;
}

static void RtspBackchannel__message_handler (GstBus * bus, GstMessage * message, gpointer p){
    GError *err;
    gchar *debug_info;
    GstState old_state, new_state, pending_state;
    
    switch(GST_MESSAGE_TYPE(message)){
        case GST_MESSAGE_UNKNOWN:
            C_TRACE("GST_MESSAGE_UNKNOWN");
            break;
        case GST_MESSAGE_EOS:
            C_ERROR ("End-Of-Stream reached.");
            break;
        case GST_MESSAGE_ERROR:
            gst_message_parse_error (message, &err, &debug_info);
            C_ERROR ("Error received from element %s: %s", GST_OBJECT_NAME (message->src), err->message);
            C_ERROR ("Debugging information: %s", debug_info ? debug_info : "none");
            g_clear_error (&err);
            g_free (debug_info);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if(GST_IS_PIPELINE(message->src)){
                gst_message_parse_state_changed (message, &old_state, &new_state, &pending_state);
                C_TRACE ("State set to %s for %s", gst_element_state_get_name (new_state), GST_OBJECT_NAME (message->src));
            }
            break;
        case GST_MESSAGE_WARNING:
        default:
            break;
    }
}

GstFlowReturn RtspBackchannel__new_sample (GstElement * appsink, RtspBackchannel * self){
    GstSample *sample;
    GstFlowReturn ret = GST_FLOW_OK;

    g_assert (self->back_stream_id != -1);

    g_signal_emit_by_name (appsink, "pull-sample", &sample);

    if (!sample)
        goto out;

#if (GST_PLUGINS_BASE_VERSION_MAJOR >= 1) && (GST_PLUGINS_BASE_VERSION_MINOR >= 21) //GST_PLUGINS_BASE_VERSION_MICRO
    g_signal_emit_by_name (self->rtspsrc, "push-backchannel-sample", self->back_stream_id, sample, &ret);
#else
    g_signal_emit_by_name (self->rtspsrc, "push-backchannel-buffer", self->back_stream_id, sample, &ret);
#endif
    //I know this was added in version 1.21, but it generates //gst_mini_object_unlock: assertion 'state >= SHARE_ONE' failed
    /* Action signal callbacks don't take ownership of the arguments passed, so we must unref the sample here.
    * (The "push-backchannel-buffer" callback unrefs the sample, which is wrong and doesn't work with bindings
    * but could not be changed, hence the new "push-backchannel-sample" callback that behaves correctly.)  */
    // gst_sample_unref (sample);
  
out:
    return ret;
}

gboolean RtspBackchannel__remove_extra_fields (GQuark field_id, GValue * value G_GNUC_UNUSED, gpointer user_data G_GNUC_UNUSED) {
    return !g_str_has_prefix (g_quark_to_string (field_id), "a-");
}