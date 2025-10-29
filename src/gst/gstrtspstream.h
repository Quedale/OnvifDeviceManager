#ifndef ONVIF_RTSPSTREAM_H_ 
#define ONVIF_RTSPSTREAM_H_

#include "portable_thread.h"
PUSH_WARNING_IGNORE(-1,-Wpedantic)
#include <gst/gst.h>
POP_WARNING_IGNORE(NULL)

G_BEGIN_DECLS

#define GST_TYPE_RTSPSTREAM GstRtspStream__get_type()
G_DECLARE_FINAL_TYPE (GstRtspStream, GstRtspStream_, GST, RTSPSTREAM, GObject)


struct _GstRtspStream {
  GObject parent_instance;
};


struct _GstRtspStreamClass {
  GObjectClass parent_class;
};

GstRtspStream * GstRtspStreams__get_stream(GList *streams, GstObject * element);
GstCaps * GstRtspStream__get_raw_caps(GstRtspStream* self);
GstCaps * GstRtspStream__get_decoded_caps(GstRtspStream* self);
GstCaps * GstRtspStream__get_depayed_caps(GstRtspStream* self);
GstTagList * GstRtspStream__get_taglist(GstRtspStream * self);

/* Internal calls meant for GstRtspPlayer */
GstRtspStream * GstRtspStream__new (GstElement * bin, GstCaps * caps);
//Internal Gstreamer probe callback to fetch stream_id and TOC
GstPadProbeReturn GstRtspStream__event_probe (GstPad * pad, GstPadProbeInfo * info,GstRtspStream * self);
//Internal GST_MESSAGE_TAG callback used to populate stream tags
void GstRtspStream__populate_tags(GstRtspStream * self, GstTagList *tl);
void GstRtspStream__set_decoder_caps(GstRtspStream * self, GstCaps * caps);
void GstRtspStream__set_depayed_caps(GstRtspStream * self, GstCaps * caps);


G_END_DECLS

#endif
