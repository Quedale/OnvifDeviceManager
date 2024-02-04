#ifndef RTSP_BACKCHANNEL_H_ 
#define RTSP_BACKCHANNEL_H_

#include "gst/gst.h"

typedef struct _RtspBackchannel RtspBackchannel;

RtspBackchannel * RtspBackchannel__create();
void RtspBackchannel__init(RtspBackchannel * self);
void RtspBackchannel__destroy(RtspBackchannel * self);

GstStateChangeReturn RtspBackchannel__pause(RtspBackchannel * self);
GstStateChangeReturn RtspBackchannel__stop(RtspBackchannel * self);
gboolean RtspBackchannel__is_mute(RtspBackchannel* self);
void RtspBackchannel__mute(RtspBackchannel* self, gboolean mute);
GstStateChangeReturn RtspBackchannel__get_state(RtspBackchannel * self, GstState * state, GstState * nstate);
gboolean RtspBackchannel__find (GstElement * rtspsrc, guint idx, GstCaps * caps, RtspBackchannel * self G_GNUC_UNUSED);
void RtspBackChannel__set_rtspsrc(RtspBackchannel * self, GstElement * src);

#endif