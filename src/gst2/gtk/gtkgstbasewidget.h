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

#ifndef __GTK_GST_BASE_CUSTOM_WIDGET_H__
#define __GTK_GST_BASE_CUSTOM_WIDGET_H__

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#define GTK_GST_BASE_CUSTOM_WIDGET(w)         ((GtkGstBaseCustomWidget *)(w))
#define GTK_GST_BASE_CUSTOM_WIDGET_CLASS(k)   ((GtkGstBaseCustomWidgetClass *)(k))
#define GTK_GST_BASE_CUSTOM_WIDGET_LOCK(w)    g_mutex_lock(&((GtkGstBaseCustomWidget*)(w))->lock)
#define GTK_GST_BASE_CUSTOM_WIDGET_UNLOCK(w)  g_mutex_unlock(&((GtkGstBaseCustomWidget*)(w))->lock)

G_BEGIN_DECLS

typedef struct _GtkGstBaseCustomWidget GtkGstBaseCustomWidget;
typedef struct _GtkGstBaseCustomWidgetClass GtkGstBaseCustomWidgetClass;

struct _GtkGstBaseCustomWidget
{
  union {
    GtkDrawingArea drawing_area;
// #if GTK_CHECK_VERSION(3, 15, 0)
//     GtkGLArea gl_area;
// #endif
  } parent;

  /* properties */
  gboolean force_aspect_ratio;
  gint par_n, par_d;
  gint video_par_n, video_par_d;
  gboolean ignore_alpha;

  gint display_width;
  gint display_height;

  gboolean negotiated;
  GstBuffer *pending_buffer;
  GstBuffer *buffer;
  GstVideoInfo v_info;

  /* resize */
  gboolean pending_resize;
  GstVideoInfo pending_v_info;
  guint display_ratio_num;
  guint display_ratio_den;

  /*< private >*/
  GMutex lock;
  GWeakRef element;

  /* Pending draw idles callback */
  guint draw_id;
};

struct _GtkGstBaseCustomWidgetClass
{
  union {
    GtkDrawingAreaClass drawing_area_class;
// #if GTK_CHECK_VERSION(3, 15, 0)
//     GtkGLAreaClass gl_area_class;
// #endif
  } parent_class;
};

/* For implementer */
void            gtk_gst_base_custom_widget_class_init           (GtkGstBaseCustomWidgetClass * klass);
void            gtk_gst_base_custom_widget_init                 (GtkGstBaseCustomWidget * widget);

void            gtk_gst_base_custom_widget_finalize             (GObject * object);

/* API */
gboolean        gtk_gst_base_custom_widget_set_format           (GtkGstBaseCustomWidget * widget, GstVideoInfo * v_info);
void            gtk_gst_base_custom_widget_set_buffer           (GtkGstBaseCustomWidget * widget, GstBuffer * buffer);
void            gtk_gst_base_custom_widget_queue_draw           (GtkGstBaseCustomWidget * widget);
void            gtk_gst_base_custom_widget_set_element          (GtkGstBaseCustomWidget * widget, GstElement * element);
void            gtk_gst_base_custom_widget_display_size_to_stream_size (GtkGstBaseCustomWidget * base_widget,
                                                                 gdouble x, gdouble y,
                                                                 gdouble * stream_x, gdouble * stream_y);

G_END_DECLS

#endif /* __GTK_GST_BASE_CUSTOM_WIDGET_H__ */
