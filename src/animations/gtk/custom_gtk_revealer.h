/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 * 
 * 
 * I've made some modification to the original GtkRevealer to support additional properties
 *  - delay
 *  - start_delay
 *  - show_delay
 *  - hide_delay
 *
 */

#ifndef __CUSTOM_GTK_REVEALER_H__
#define __CUSTOM_GTK_REVEALER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define CUSTOM_GTK_TYPE_REVEALER (custom_gtk_revealer_get_type ())
#define CUSTOM_GTK_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUSTOM_GTK_TYPE_REVEALER, CustomGtkRevealer))
#define CUSTOM_GTK_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CUSTOM_GTK_TYPE_REVEALER, CustomGtkRevealerClass))
#define CUSTOM_IS_GTK_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_GTK_TYPE_REVEALER))
#define CUSTOM_IS_GTK_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CUSTOM_GTK_TYPE_REVEALER))
#define CUSTOM_GTK_REVEALER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CUSTOM_GTK_TYPE_REVEALER, CustomGtkRevealerClass))

typedef struct _CustomGtkRevealer CustomGtkRevealer;
typedef struct _CustomGtkRevealerClass CustomGtkRevealerClass;

typedef enum {
  CUSTOM_GTK_REVEALER_TRANSITION_TYPE_NONE,
  CUSTOM_GTK_REVEALER_TRANSITION_TYPE_CROSSFADE,
  CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT,
  CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT,
  CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP,
  CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
} CustomGtkRevealerTransitionType;

struct _CustomGtkRevealer {
  GtkBin parent_instance;
};

/**
 * CustomGtkRevealerClass:
 * @parent_class: The parent class.
 */
struct _CustomGtkRevealerClass {
  GtkBinClass parent_class;
};

GDK_AVAILABLE_IN_3_10
GType                      custom_gtk_revealer_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_10
GtkWidget*                 custom_gtk_revealer_new                     (void);
GDK_AVAILABLE_IN_3_10
gboolean                   custom_gtk_revealer_get_reveal_child        (CustomGtkRevealer               *revealer);
GDK_AVAILABLE_IN_3_10
void                       custom_gtk_revealer_set_reveal_child        (CustomGtkRevealer               *revealer,
                                                                 gboolean                   reveal_child);
GDK_AVAILABLE_IN_3_10
gboolean                   custom_gtk_revealer_get_child_revealed      (CustomGtkRevealer               *revealer);
GDK_AVAILABLE_IN_3_10
guint                      custom_gtk_revealer_get_transition_duration (CustomGtkRevealer               *revealer);
GDK_AVAILABLE_IN_3_10
void                       custom_gtk_revealer_set_transition_duration (CustomGtkRevealer               *revealer,
                                                                 guint                      duration);
GDK_AVAILABLE_IN_3_10
void                       custom_gtk_revealer_set_transition_type     (CustomGtkRevealer               *revealer,
                                                                 CustomGtkRevealerTransitionType  transition);
GDK_AVAILABLE_IN_3_10
CustomGtkRevealerTransitionType  custom_gtk_revealer_get_transition_type     (CustomGtkRevealer               *revealer);

void custom_gtk_revealer_set_start_delay (CustomGtkRevealer *revealer, guint     delay);

guint custom_gtk_revealer_get_start_delay (CustomGtkRevealer *revealer);

void custom_gtk_revealer_set_show_delay (CustomGtkRevealer *revealer, guint     delay);

guint custom_gtk_revealer_get_show_delay (CustomGtkRevealer *revealer);

void custom_gtk_revealer_set_hide_delay (CustomGtkRevealer *revealer, guint     delay);

guint custom_gtk_revealer_get_hide_delay (CustomGtkRevealer *revealer);

void custom_gtk_revealer_set_delay (CustomGtkRevealer *revealer, guint     delay);

guint custom_gtk_revealer_get_delay (CustomGtkRevealer *revealer);

G_END_DECLS

#endif
