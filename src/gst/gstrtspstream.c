#include "gstrtspstream.h"
#include "clogger.h"

enum {
  NEW_TAGS,
  LAST_SIGNAL
};

typedef struct {
    GstElement * bin;
    GstElement * sink;
    GstCaps * raw_caps;
    GstCaps * depayed_caps;
    GstCaps * decoded_caps;
    GstToc *toc;
    GstTagList *tags;
    gchar *stream_id;

    P_MUTEX_TYPE tags_lock;
} GstRtspStreamPrivate;

typedef struct {
    GObject * object;
    guint signalid;
    GstTagList * diff;
} StreamSignalData;

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(GstRtspStream, GstRtspStream_, G_TYPE_OBJECT)

static gboolean 
_GstRtspStream__signal_emit(StreamSignalData * data){
    g_signal_emit (data->object, data->signalid, 0, data->diff);
    g_object_unref(data->object);
    free(data);
    return FALSE;
}

static void 
GstRtspStream__idle_signal_emit(GstRtspStream * self, guint signalid, GstTagList * diff){
    StreamSignalData * data = malloc(sizeof(StreamSignalData));
    data->object = G_OBJECT(self);
    data->signalid = signalid;
    data->diff = diff;
    g_object_ref(self);
    g_main_context_invoke(g_main_context_default(),G_SOURCE_FUNC(_GstRtspStream__signal_emit),data);
}

static gboolean 
gst_tag_list_has_value(GstTagList *tl, const char * tagname, GType type, void ** value){
    if(!tl){
        return FALSE;
    }
    gboolean ret = FALSE;
    void * void_value = NULL;
    size_t size = 0;
    for(guint a=0;a<gst_tag_list_get_tag_size(tl,tagname);a++){
        switch(type){
            case G_TYPE_STRING:
                if(!gst_tag_list_peek_string_index(tl,tagname,a,(const char**)&void_value))
                    return FALSE;
                if(void_value && !strcmp(void_value,*value))
                    return TRUE;
                break;
            case G_TYPE_INT:
                size = sizeof(gint);
                void_value = malloc(size);
                if(!gst_tag_list_get_int_index(tl,tagname,a,(gint *)void_value))
                    return FALSE;
                break;
            case G_TYPE_UINT:
                size = sizeof(guint);
                void_value = malloc(size);
                if(!gst_tag_list_get_uint_index(tl,tagname,a,(guint *)void_value))
                    return FALSE;
                break;
            case G_TYPE_UINT64:
                size = sizeof(guint64);
                void_value = malloc(size);
                if(!gst_tag_list_get_uint64_index(tl,tagname,a,(guint64 *)void_value))
                    return FALSE;
                break;
            case G_TYPE_INT64:
                size = sizeof(gint64);
                void_value = malloc(size);
                if(!gst_tag_list_get_int64_index(tl,tagname,a,(gint64 *)void_value))
                    return FALSE;
                break;
            case G_TYPE_BOOLEAN:
                size = sizeof(gboolean);
                void_value = malloc(size);
                if(!gst_tag_list_get_boolean_index(tl,tagname,a,(gboolean *)void_value))
                    return FALSE;
                break;
            case G_TYPE_FLOAT:
                size = sizeof(gfloat);
                void_value = malloc(size);
                if(!gst_tag_list_get_float_index(tl,tagname,a,(gfloat *)void_value))
                    return FALSE;
                break;
            case G_TYPE_DOUBLE:
            case G_TYPE_POINTER:
            // case gst_date_get_type():
            // case gst_date_time_get_type():
            // case gst_sample_get_type():
            default:
                break;
        }

        //Compare value memory since pointer address won't match
        if(void_value && !memcmp(value, (int*)void_value, size)){
            free(void_value);
            ret = TRUE;
            break;
        }

        if(void_value){
            free(void_value);
        }
    }

    return ret;
}

GstRtspStream *
GstRtspStreams__get_stream(GList *streams, GstObject * element){
    g_return_val_if_fail(streams != NULL, NULL);

    for (GList * curr = streams; curr; curr = curr->next) {
        GstRtspStream *ps = GST_RTSPSTREAM(curr->data);
        GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (ps);
        if (element == GST_OBJECT_CAST (priv->bin) || // video_bin - audio_bin
            GST_OBJECT_PARENT(GST_OBJECT_CAST(element)) == GST_OBJECT_CAST(priv->bin) || // glsinkbin - autoaudiosink
            GST_OBJECT_PARENT(GST_OBJECT_PARENT(GST_OBJECT_CAST(element))) == GST_OBJECT_CAST(priv->bin)) { // gtkglsink - pulsesink
            return ps;
        }
    }

    return NULL;
}

void 
GstRtspStream__set_decoder_caps(GstRtspStream * self, GstCaps * caps){
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    priv->decoded_caps = caps;
}

void
GstRtspStream__set_depayed_caps(GstRtspStream * self, GstCaps * caps){
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    priv->depayed_caps = caps;
}

void
GstRtspStream__populate_tags(GstRtspStream * self, GstTagList *tl){
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);

    GstTagList * diff = gst_tag_list_new_empty ();
    GValue new_tag_value = G_VALUE_INIT;

    //Create a new list containing only the modified/new tags
    for (int i=0;i<gst_tag_list_n_tags(tl);i++){
        g_value_unset (&new_tag_value); //Reset value
        const gchar * tagname = gst_tag_list_nth_tag_name(tl,i);
        GType type = gst_tag_get_type(tagname);
        for(guint a=0;a<gst_tag_list_get_tag_size(tl,tagname);a++){
            switch(type){
                case G_TYPE_STRING:
                    const gchar * str_value;
                    if(gst_tag_list_peek_string_index(tl,tagname,a,&str_value) && !gst_tag_list_has_value(priv->tags,tagname,type,(void*)&str_value)){
                        g_value_init (&new_tag_value, type);
                        g_value_set_static_string (&new_tag_value, str_value);
                        gst_tag_list_add_value(diff,GST_TAG_MERGE_REPLACE,tagname,&new_tag_value);
                    }
                    break;
                case G_TYPE_INT:
                    gint gint_value;
                    if(gst_tag_list_get_int_index(tl,tagname,a,&gint_value) && !gst_tag_list_has_value(priv->tags,tagname,type,(void*)&gint_value)){
                        g_value_init (&new_tag_value, type);
                        g_value_set_int (&new_tag_value, gint_value);
                        gst_tag_list_add_value(diff,GST_TAG_MERGE_REPLACE,tagname,&new_tag_value);
                    }
                    break;
                case G_TYPE_UINT:
                    guint guint_value;
                    if(gst_tag_list_get_uint_index(tl,tagname,a,&guint_value) && !gst_tag_list_has_value(priv->tags,tagname,type,(void*)&guint_value)){
                        g_value_init (&new_tag_value, type);
                        g_value_set_uint (&new_tag_value, guint_value);
                        gst_tag_list_add_value(diff,GST_TAG_MERGE_REPLACE,tagname,&new_tag_value);
                    }
                    break;
                case G_TYPE_UINT64:
                case G_TYPE_INT64:
                case G_TYPE_BOOLEAN:
                case G_TYPE_FLOAT:
                case G_TYPE_DOUBLE:
                case G_TYPE_POINTER:
                // case gst_date_get_type():
                // case gst_date_time_get_type():
                // case gst_sample_get_type():
                default:
                    C_FIXME("Unexpected tag type : %s for '%s'",g_type_name(type), tagname);
                    break;
            }
        }
    }

    //Merge the new tags with the old ones
    P_MUTEX_LOCK(priv->tags_lock);
    GstTagList *tmp = gst_tag_list_merge (priv->tags, tl, GST_TAG_MERGE_REPLACE);
    if (priv->tags)
        gst_tag_list_unref (priv->tags);
    priv->tags = tmp;
    P_MUTEX_UNLOCK(priv->tags_lock);

    //Signal listeners about new/modified tags
    if(!gst_tag_list_is_empty(diff)){
        GstRtspStream__idle_signal_emit(self,signals[NEW_TAGS],diff);
    } else {
        gst_tag_list_unref (diff);
    }
}

GstPadProbeReturn
GstRtspStream__event_probe (GstPad * pad, GstPadProbeInfo * info,GstRtspStream * self){
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_TOC:
            GstToc *tmp;

            gst_event_parse_toc (event, &tmp, NULL);
            priv->toc = tmp;

            break;
        case GST_EVENT_STREAM_START:
            const gchar *stream_id;

            gst_event_parse_stream_start (event, &stream_id);

            g_free (priv->stream_id);
            priv->stream_id = stream_id ? g_strdup (stream_id) : NULL;
            break;
        default:
            break;
    }

    return GST_PAD_PROBE_OK;
}

static void
GstRtspStream__dispose (GObject *gobject){
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (GST_RTSPSTREAM(gobject));
    P_MUTEX_CLEANUP(priv->tags_lock);

    if(priv->decoded_caps){
        gst_caps_unref(priv->decoded_caps);
        priv->decoded_caps = NULL;
    }

    if(priv->depayed_caps){
        gst_caps_unref(priv->depayed_caps);
        priv->depayed_caps = NULL;
    }

    if(priv->raw_caps){
        gst_caps_unref(priv->raw_caps);
        priv->raw_caps = NULL;
    }

    if(priv->tags){
        gst_tag_list_unref(priv->tags);
        priv->tags = NULL;
    }

    if(priv->stream_id){
        g_free(priv->stream_id);
        priv->stream_id = NULL;
    }

    if(priv->toc){
        gst_toc_unref(priv->toc);
        priv->toc = NULL;
    }

    G_OBJECT_CLASS (GstRtspStream__parent_class)->dispose (gobject);
}

static void
GstRtspStream__class_init (GstRtspStreamClass * klass){

    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = GstRtspStream__dispose;

    GType params[1];
    params[0] = gst_tag_list_get_type() | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[NEW_TAGS] =
        g_signal_newv ("new-tags",
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

static void
GstRtspStream__init (GstRtspStream * self){
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    priv->bin = NULL;
    priv->raw_caps = NULL;
    priv->decoded_caps = NULL;
    priv->depayed_caps = NULL;
    priv->raw_caps = NULL;
    priv->stream_id = NULL;
    priv->tags = NULL;
    priv->toc = NULL;
    P_MUTEX_SETUP(priv->tags_lock);
}

GstRtspStream*  
GstRtspStream__new (GstElement * bin, GstCaps * caps){
    GstRtspStream * stream = g_object_new (GST_TYPE_RTSPSTREAM, NULL);
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (stream);
    priv->bin = bin;
    priv->raw_caps = caps;
    return stream;
}

GstCaps * 
GstRtspStream__get_raw_caps(GstRtspStream* self){
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(GST_IS_RTSPSTREAM(self), NULL);

    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    return priv->raw_caps;
}

GstCaps * 
GstRtspStream__get_decoded_caps(GstRtspStream* self){
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(GST_IS_RTSPSTREAM(self), NULL);

    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    return priv->decoded_caps;
}

GstCaps * 
GstRtspStream__get_depayed_caps(GstRtspStream* self){
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(GST_IS_RTSPSTREAM(self), NULL);

    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    return priv->depayed_caps;
}

GstTagList * 
GstRtspStream__get_taglist(GstRtspStream * self){
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(GST_IS_RTSPSTREAM(self), NULL);
    GstRtspStreamPrivate *priv = GstRtspStream__get_instance_private (self);
    P_MUTEX_LOCK(priv->tags_lock);
    GstTagList * ret = NULL;
    if(priv->tags)
        ret = gst_tag_list_copy(priv->tags); //Creating a copy so that streaming thread can keep merging new tags while the UI updates
    P_MUTEX_UNLOCK(priv->tags_lock);
    return ret;
}