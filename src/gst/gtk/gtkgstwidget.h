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

#ifndef __GTK_GST_CUSTOM_WIDGET_H__
#define __GTK_GST_CUSTOM_WIDGET_H__

#include <gtk/gtk.h>
#include <gst/gst.h>

#include "gtkgstbasewidget.h"

G_BEGIN_DECLS

GType gtk_gst_custom_widget_get_type (void);
#define GTK_TYPE_GST_CUSTOM_WIDGET (gtk_gst_custom_widget_get_type())
#define GTK_GST_CUSTOM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GTK_TYPE_GST_CUSTOM_WIDGET,GtkGstCustomWidget))
#define GTK_GST_CUSTOM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GTK_TYPE_GST_CUSTOM_WIDGET,GtkGstCustomWidgetClass))
#define GTK_IS_GST_CUSTOM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GTK_TYPE_GST_CUSTOM_WIDGET))
#define GST_IS_GST_CUSTOM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GTK_TYPE_GST_CUSTOM_WIDGET))
#define GTK_GST_CUSTOM_WIDGET_CAST(obj) ((GtkGstCustomWidget*)(obj))

typedef struct _GtkGstCustomWidget GtkGstCustomWidget;
typedef struct _GtkGstCustomWidgetClass GtkGstCustomWidgetClass;

/**
 * GtkGstCustomWidget:
 *
 * Opaque #GtkGstCustomWidget object
 */
struct _GtkGstCustomWidget
{
  /* <private> */
  GtkGstBaseCustomWidget base;
};

/**
 * GtkGstCustomWidgetClass:
 *
 * The #GtkGstCustomWidgetClass struct only contains private data
 */
struct _GtkGstCustomWidgetClass
{
  /* <private> */
  GtkGstBaseCustomWidgetClass base_class;
};

GtkWidget *     gtk_gst_custom_widget_new (void);

G_END_DECLS

#endif /* __GTK_GST_CUSTOM_WIDGET_H__ */
