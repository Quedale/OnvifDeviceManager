#include "onvif_nvt_stream_details.h"
#include "clogger.h"
#include "gui_utils.h"
#include <gst/pbutils/descriptions.h>

enum {
  PROP_TAG = 1,
  N_PROPERTIES
};

enum {
  PROP_STREAM = 1,
  NX_PROPERTIES
};

typedef struct {
    char * tagname;
} OnvifNVTTagPanelPrivate;

typedef struct {
    GtkWidget * tags_box;
    int tags_count;
    OnvifNVTTagPanel ** tag_boxes;
    GstRtspStream * stream;
    gulong tag_handler;
} OnvifNVTStreamDetailsPrivate;

static GParamSpec *tag_properties[N_PROPERTIES] = { NULL, };
static GParamSpec *stream_properties[NX_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(OnvifNVTTagPanel, OnvifNVTTagPanel_, GTK_TYPE_BOX)
G_DEFINE_TYPE_WITH_PRIVATE(OnvifNVTStreamDetails, OnvifNVTStreamDetails_, GTK_TYPE_GRID)

static void
OnvifNVTTagPanel__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifNVTTagPanel * self = ONVIFMGR_NVTTAGPANEL (object);
    OnvifNVTTagPanelPrivate *priv = OnvifNVTTagPanel__get_instance_private (self);

    switch (prop_id){
        case PROP_TAG:
            const char * strval = g_value_get_string (value);
            if(priv->tagname)
                free(priv->tagname);
            priv->tagname = malloc(strlen(strval) + 1);
            strcpy((char *)priv->tagname,strval);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifNVTTagPanel__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifNVTTagPanel * self = ONVIFMGR_NVTTAGPANEL (object);
    OnvifNVTTagPanelPrivate *priv = OnvifNVTTagPanel__get_instance_private (self);
    switch (prop_id){
        case PROP_TAG:
            g_value_set_string (value, priv->tagname);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifNVTTagPanel__class_init (OnvifNVTTagPanelClass * klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifNVTTagPanel__set_property;
    object_class->get_property = OnvifNVTTagPanel__get_property;

    tag_properties[PROP_TAG] =
        g_param_spec_string ("tagname",
                            "GstTagName",
                            "The gstreamer tag name",
                            "TagName",
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        tag_properties);
}

OnvifNVTTagPanel * 
OnvifNVTTagPanel__new(const char * tagname){
    return g_object_new (ONVIFMGR_TYPE_NVTTAGPANEL, "tagname", tagname, "orientation", GTK_ORIENTATION_VERTICAL, "spacing", 0, NULL);
}

static void
OnvifNVTTagPanel__init (OnvifNVTTagPanel * self){
    OnvifNVTTagPanelPrivate *priv = OnvifNVTTagPanel__get_instance_private (self);
    priv->tagname = NULL;
}

const char *
OnvifNVTTagPanel__get_tagname (OnvifNVTTagPanel * self){
    OnvifNVTTagPanelPrivate *priv = OnvifNVTTagPanel__get_instance_private (self);
    return priv->tagname;
}

static void
OnvifNVTStreamDetails__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifNVTStreamDetails * self = ONVIFMGR_NVTSTREAMDETAILS (object);
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);

    switch (prop_id){
        case PROP_STREAM:
            priv->stream = g_value_get_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifNVTStreamDetails__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifNVTStreamDetails * self = ONVIFMGR_NVTSTREAMDETAILS (object);
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    switch (prop_id){
        case PROP_STREAM:
            g_value_set_object (value, priv->stream);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static OnvifNVTTagPanel * OnvifNVTStreamDetails__get_tag_box_with_label(OnvifNVTStreamDetails * self, const gchar * tagname, const gchar * strlabel){
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    if(!priv->tag_boxes){ //No tag ever created yet, initalize pointer and skip to creating panel
        priv->tag_boxes = malloc(sizeof(OnvifNVTTagPanel *) * ++priv->tags_count);
        goto create_panel;
    }

    //Check for existing panel for this tag
    for(int i=0;i<priv->tags_count;i++){
        OnvifNVTTagPanel * panel = priv->tag_boxes[i];
        if(!strcmp(OnvifNVTTagPanel__get_tagname(panel),tagname)){
            return panel;
        }
    }
    
    //No panel found for this tag, increase allocation size and create new panel
    priv->tag_boxes = realloc(priv->tag_boxes, sizeof(OnvifNVTTagPanel *) * ++priv->tags_count);

create_panel:
    OnvifNVTTagPanel * panel = OnvifNVTTagPanel__new(tagname);
    priv->tag_boxes[priv->tags_count-1] = panel;
    GtkWidget * label = gtk_label_new(strlabel);
    gtk_widget_set_margin_end(label,10);
    gtk_widget_set_halign(label,GTK_ALIGN_START);
    gtk_grid_attach (GTK_GRID (priv->tags_box), label, 0, priv->tags_count, 1, 1);
    gtk_grid_attach (GTK_GRID (priv->tags_box), GTK_WIDGET(panel), 1, priv->tags_count, 1, 1);
    return panel;
}

static OnvifNVTTagPanel * 
OnvifNVTStreamDetails__get_tag_box(OnvifNVTStreamDetails * self, const gchar * tagname){
    return OnvifNVTStreamDetails__get_tag_box_with_label(self,tagname,tagname);
}

static void
OnvifNVTStreamDetails__tags_changed(GstRtspStream * stream, GstTagList * diff_tags, OnvifNVTStreamDetails * self){
    if(!diff_tags){ //This is possible if the panel is visible before any tags are received
        return;
    }

    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    for (int i=0;i<gst_tag_list_n_tags(diff_tags);i++){ 
        const gchar * tagname = gst_tag_list_nth_tag_name(diff_tags,i);

        OnvifNVTTagPanel * tag_panel = OnvifNVTStreamDetails__get_tag_box(self,tagname);
        gtk_container_foreach(GTK_CONTAINER(tag_panel),(GtkCallback)gui_widget_destroy, NULL);

        GType type = gst_tag_get_type(tagname);
        for(guint a=0;a<gst_tag_list_get_tag_size(diff_tags,tagname);a++){
            switch(type){
                case G_TYPE_STRING:
                    const gchar * str_value;
                    if(gst_tag_list_peek_string_index(diff_tags,tagname,a,&str_value)){
                        GtkWidget * label = gtk_label_new(str_value);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_INT:
                    gint gint_value;
                    if(gst_tag_list_get_int_index(diff_tags,tagname,a,&gint_value)){
                        char ch[25];
                        sprintf(ch, "%d", gint_value);
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_UINT:
                    guint guint_value;
                    if(gst_tag_list_get_uint_index(diff_tags,tagname,a,&guint_value)){
                        char ch[25];
                        sprintf(ch, "%u", guint_value);
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_UINT64:
                    guint64 guint64_value;
                    if(gst_tag_list_get_uint64_index(diff_tags,tagname,a,&guint64_value)){
                        char ch[25];
                        sprintf(ch, "%lu", guint64_value);
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_INT64:
                    gint64 gint64_value;
                    if(gst_tag_list_get_int64_index(diff_tags,tagname,a,&gint64_value)){
                        char ch[25];
                        sprintf(ch, "%ld", gint64_value);
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_BOOLEAN:
                    gboolean gbool_value;
                    if(gst_tag_list_get_boolean_index(diff_tags,tagname,a,&gbool_value)){
                        char * ch;
                        if(gbool_value)
                            ch = "True";
                        else
                            ch = "False";
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_FLOAT:
                    gfloat gfloat_value;
                    if(gst_tag_list_get_float_index(diff_tags,tagname,a,&gfloat_value)){
                        char ch[25];
                        sprintf(ch, "%f", gfloat_value);
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
                case G_TYPE_DOUBLE:
                    gdouble gdouble_value;
                    if(gst_tag_list_get_double_index(diff_tags,tagname,a,&gdouble_value)){
                        char ch[25];
                        sprintf(ch, "%.2f", gdouble_value);
                        GtkWidget * label = gtk_label_new((char*)ch);
                        gtk_widget_set_halign(label,GTK_ALIGN_START);
                        gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
                    }
                    break;
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
    gtk_widget_show_all(priv->tags_box);
    gst_tag_list_unref (diff_tags);
}

static void 
OnvifNVTStreamDetails__create_panel_from_caps(OnvifNVTStreamDetails * self, const char * media_type){
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    GstCaps * decoded_caps = GstRtspStream__get_decoded_caps(priv->stream);
    GstCaps * depayed_caps = GstRtspStream__get_depayed_caps(priv->stream);
    // GstCaps * raw_caps = GstRtspStream__get_raw_caps(priv->stream);

    char * codec_desc = gst_pb_utils_get_codec_description(depayed_caps);
    char codec_key[14];
    sprintf(codec_key,"%s-codec",media_type);
    OnvifNVTTagPanel * tag_panel = OnvifNVTStreamDetails__get_tag_box_with_label(self,codec_key,"codec");
    GtkWidget * label = gtk_label_new(codec_desc);
    gtk_widget_set_halign(label,GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);

    codec_desc = gst_pb_utils_get_codec_description(decoded_caps);
    tag_panel = OnvifNVTStreamDetails__get_tag_box(self,"decoded");
    label = gtk_label_new(codec_desc);
    gtk_widget_set_halign(label,GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);  

    //Display decoded caps details 
    GstStructure * structure;
    for(guint i=0;i<gst_caps_get_size(decoded_caps);i++){
        structure = gst_caps_get_structure(decoded_caps,i);
        gint width = 0;
        if(gst_structure_has_field(structure,"width")){
            if(!gst_structure_get_int(structure,"width", &width)){
                C_WARN("Failed to extract width from caps");
            }
        }

        gint height = 0;
        if(gst_structure_has_field(structure,"height")){
            if(!gst_structure_get_int(structure,"height", &height)){
                C_WARN("Failed to extract height from caps");
            }
        }

        if(height > 0 || width > 0){
            tag_panel = OnvifNVTStreamDetails__get_tag_box(self,"resolution");
            char rate_str[12];
            sprintf(rate_str, "%d/%d", width,height);
            label = gtk_label_new(rate_str);
            gtk_widget_set_halign(label,GTK_ALIGN_START);
            gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);  
        }

        if(gst_structure_has_field(structure,"framerate")){
            gint value_numerator = 0;
            gint value_denominator = 0;
            if(gst_structure_get_fraction(structure,"framerate", &value_numerator, &value_denominator)){
                tag_panel = OnvifNVTStreamDetails__get_tag_box(self,"framerate");

                char rate_str[10];
                sprintf(rate_str, "%d/%d", value_numerator, value_denominator);
                label = gtk_label_new(rate_str);
                gtk_widget_set_halign(label,GTK_ALIGN_START);
                gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
            }
        }
    }

    //Display depayed caps
    for(guint i=0;i<gst_caps_get_size(depayed_caps);i++){
        structure = gst_caps_get_structure(depayed_caps,i);
        if(gst_structure_has_field(structure,"rate")){
            gint value;
            if(gst_structure_get_int(structure,"rate", &value)){
                tag_panel = OnvifNVTStreamDetails__get_tag_box(self,"rate");

                char rate_str[12];
                sprintf(rate_str, "%d", value);
                label = gtk_label_new(rate_str);
                gtk_widget_set_halign(label,GTK_ALIGN_START);
                gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
            }
        }
        if(gst_structure_has_field(structure,"channels")){
            gint value;
            if(gst_structure_get_int(structure,"channels", &value)){
                tag_panel = OnvifNVTStreamDetails__get_tag_box(self,"channels");

                char rate_str[12];
                sprintf(rate_str, "%d", value);
                label = gtk_label_new(rate_str);
                gtk_widget_set_halign(label,GTK_ALIGN_START);
                gtk_box_pack_start (GTK_BOX (tag_panel), label, FALSE, FALSE, 0);
            }
        }
    }
}

static void
OnvifNVTStreamDetails__constructed (GObject    *object){
    OnvifNVTStreamDetails * self = ONVIFMGR_NVTSTREAMDETAILS(object);
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    GtkWidget * type_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_grid_attach (GTK_GRID (object), type_box, 0, 0, 1, 1);
    
    priv->tags_box = gtk_grid_new();
    gtk_widget_set_margin_start(priv->tags_box,20);

    GstCaps * raw_caps = GstRtspStream__get_raw_caps(priv->stream);
    GstCaps * decoded_caps = GstRtspStream__get_decoded_caps(priv->stream);
    GstCaps * depayed_caps = GstRtspStream__get_depayed_caps(priv->stream);

    gchar *caps_str = gst_caps_to_string(raw_caps);
    C_DEBUG("RTP Caps: %s", caps_str);
    g_free(caps_str);
    caps_str = gst_caps_to_string(depayed_caps);
    C_DEBUG("Depayed Caps: %s", caps_str);
    g_free(caps_str);
    caps_str = gst_caps_to_string(decoded_caps);
    C_DEBUG("Decoded Caps: %s", caps_str);
    g_free(caps_str);

    GstStructure * structure;
    for(guint i=0;i<gst_caps_get_size(raw_caps);i++){
        structure = gst_caps_get_structure(raw_caps,i);
        const char * media_type = gst_structure_get_string(structure, "media");
        GtkWidget * label = gtk_label_new(media_type);
        gtk_widget_set_halign(label,GTK_ALIGN_START);
        gtk_box_pack_start (GTK_BOX (type_box), label, FALSE, TRUE, 0);

        OnvifNVTStreamDetails__create_panel_from_caps(self,media_type);
    }

    gtk_box_pack_start (GTK_BOX (type_box), priv->tags_box, FALSE, TRUE, 0);

    G_OBJECT_CLASS (OnvifNVTStreamDetails__parent_class)->constructed (object);
}

static void
OnvifNVTStreamDetails__dispose(GObject * gobject){
    OnvifNVTStreamDetails * self = ONVIFMGR_NVTSTREAMDETAILS(gobject);
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    if(priv->tag_boxes){
        free(priv->tag_boxes);
        priv->tag_boxes = NULL;
    }
    G_OBJECT_CLASS (OnvifNVTStreamDetails__parent_class)->dispose (gobject);
}

static void
OnvifNVTStreamDetails__map (GtkWidget *widget){
    OnvifNVTStreamDetails * self = ONVIFMGR_NVTSTREAMDETAILS(widget);
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);

    //When back on the UI, update tags with the stream list
    GstTagList * tl = GstRtspStream__get_taglist(priv->stream);
    if(tl){
        OnvifNVTStreamDetails__tags_changed(priv->stream,tl,self);
        gtk_widget_show_all(widget);
    }

    //Register new-tags signal to keep updating new tag values
    priv->tag_handler = g_signal_connect (G_OBJECT(priv->stream), "new-tags", G_CALLBACK (OnvifNVTStreamDetails__tags_changed), self);

    GTK_WIDGET_CLASS (OnvifNVTStreamDetails__parent_class)->map (widget);
}

static void
OnvifNVTStreamDetails__unmap (GtkWidget * widget){
    OnvifNVTStreamDetails * self = ONVIFMGR_NVTSTREAMDETAILS(widget);
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);

    //Remove new-tags handler since the GUI isn't displaying it anymore
    if(priv->tag_handler > 0){
        g_signal_handler_disconnect(priv->stream,priv->tag_handler);
        priv->tag_handler =0;
    }
    
    GTK_WIDGET_CLASS (OnvifNVTStreamDetails__parent_class)->unmap (widget);
}

static void
OnvifNVTStreamDetails__class_init (OnvifNVTStreamDetailsClass * klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifNVTStreamDetails__set_property;
    object_class->get_property = OnvifNVTStreamDetails__get_property;
    object_class->constructed = OnvifNVTStreamDetails__constructed;
    object_class->dispose = OnvifNVTStreamDetails__dispose;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->map = OnvifNVTStreamDetails__map;
    widget_class->unmap = OnvifNVTStreamDetails__unmap;

    stream_properties[PROP_STREAM] =
        g_param_spec_object ("stream",
                            "GstRtspStream",
                            "The GstRtspStream to display details for.",
                            GST_TYPE_RTSPSTREAM,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
                                        NX_PROPERTIES,
                                        stream_properties);
}

static void
OnvifNVTStreamDetails__init (OnvifNVTStreamDetails * self){
    OnvifNVTStreamDetailsPrivate *priv = OnvifNVTStreamDetails__get_instance_private (self);
    priv->tag_boxes = NULL;
    priv->stream = NULL;
    priv->tags_box = NULL;
    priv->tags_count = 0;
    priv->tag_handler = 0;
}

GtkWidget* 
OnvifNVTStreamDetails__new(GstRtspStream * stream){
    return g_object_new (ONVIFMGR_TYPE_NVTSTREAMDETAILS, "stream", stream, NULL);
}