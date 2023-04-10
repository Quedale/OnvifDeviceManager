/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_GTK_BASE_CUSTOM_SINK_H__
#define __GST_GTK_BASE_CUSTOM_SINK_H__

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

#include "gtkgstbasewidget.h"

#define GST_TYPE_GTK_BASE_CUSTOM_SINK            (gst_gtk_base_custom_sink_get_type())
#define GST_GTK_BASE_CUSTOM_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GTK_BASE_CUSTOM_SINK,GstGtkBaseCustomSink))
#define GST_GTK_BASE_CUSTOM_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GTK_BASE_CUSTOM_SINK,GstGtkBaseCustomSinkClass))
#define GST_GTK_BASE_CUSTOM_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GTK_BASE_CUSTOM_SINK, GstGtkBaseCustomSinkClass))
#define GST_IS_GTK_BASE_CUSTOM_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GTK_BASE_CUSTOM_SINK))
#define GST_IS_GTK_BASE_CUSTOM_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GTK_BASE_CUSTOM_SINK))
#define GST_GTK_BASE_CUSTOM_SINK_CAST(obj)       ((GstGtkBaseCustomSink*)(obj))

G_BEGIN_DECLS

typedef struct _GstGtkBaseCustomSink GstGtkBaseCustomSink;
typedef struct _GstGtkBaseCustomSinkClass GstGtkBaseCustomSinkClass;

GType gst_gtk_base_custom_sink_get_type (void);

/**
 * GstGtkBaseCustomSink:
 *
 * Opaque #GstGtkBaseCustomSink object
 */
struct _GstGtkBaseCustomSink
{
  /* <private> */
  GstVideoSink         parent;

  GstVideoInfo         v_info;

  GtkGstBaseCustomWidget     *widget;
  GtkWidget            *widget_parent;

  /* properties */
  gboolean             expand;
  gboolean             force_aspect_ratio;
  GBinding             *bind_aspect_ratio;

  gint                  par_n;
  gint                  par_d;
  GBinding             *bind_pixel_aspect_ratio;

  gint                  video_par_n;
  gint                  video_par_d;
  GBinding             *bind_video_aspect_ratio;

  gboolean              ignore_alpha;
  GBinding             *bind_ignore_alpha;

  GtkWidget            *window;
  gulong               widget_destroy_id;
  gulong               window_destroy_id;
};

/**
 * GstGtkBaseCustomSinkClass:
 *
 * The #GstGtkBaseCustomSinkClass struct only contains private data
 */
struct _GstGtkBaseCustomSinkClass
{
  GstVideoSinkClass object_class;

  // /* metadata */
  // const gchar *window_title;

  /* virtuals */
  GtkWidget* (*create_widget) (void);
};

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GstGtkBaseCustomSink, gst_object_unref)

GtkWidget *
gst_gtk_base_custom_sink_acquire_widget (GstGtkBaseCustomSink * gtk_sink);
GtkGstBaseCustomWidget *
gst_gtk_base_custom_sink_get_widget (GstGtkBaseCustomSink * gtk_sink);

GtkGstBaseCustomWidget *
gst_gtk_base_custom_sink_set_widget (GstGtkBaseCustomSink * gtk_sink,GtkGstBaseCustomWidget * widget);

void
gst_gtk_base_custom_sink_set_parent (GstGtkBaseCustomSink * gtk_sink, GtkWidget * parent);

void 
gst_gtk_base_custom_sink_set_expand(GstGtkBaseCustomSink * bsink, gboolean val);
G_END_DECLS

#endif /* __GST_GTK_BASE_CUSTOM_SINK_H__ */
