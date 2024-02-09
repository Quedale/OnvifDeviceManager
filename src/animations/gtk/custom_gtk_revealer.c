/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright 2013, 2015 Red Hat, Inc.
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
 *         Carlos Soriano <csoriano@gnome.org>
 */

// #include "config.h"
#include "custom_gtk_revealer.h"
#include "custom_gtk_progress_tracker.h"

#include <gtk/gtk.h>
#include <math.h>
// #include <gdk/gdk.h>
// #include "gtktypebuiltins.h"
// #include "gtkprivate.h"
// #include "gtksettingsprivate.h"
// #include "gtkprogresstrackerprivate.h"
// #include "gtkintl.h"

// #include "fallback-c89.c"
// #include <gtk/gtk.h>

#define P_(x) x
#define CUSTOM_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

/**
 * SECTION:gtkrevealer
 * @Short_description: Hide and show with animation
 * @Title: CustomGtkRevealer
 * @See_also: #GtkExpander
 *
 * The CustomGtkRevealer widget is a container which animates
 * the transition of its child from invisible to visible.
 *
 * The style of transition can be controlled with
 * custom_gtk_revealer_set_transition_type().
 *
 * These animations respect the #GtkSettings:gtk-enable-animations
 * setting.
 *
 * # CSS nodes
 *
 * CustomGtkRevealer has a single CSS node with name revealer.
 *
 * The CustomGtkRevealer widget was added in GTK+ 3.10.
 */

/**
 * CustomGtkRevealerTransitionType:
 * @CUSTOM_GTK_REVEALER_TRANSITION_TYPE_NONE: No transition
 * @CUSTOM_GTK_REVEALER_TRANSITION_TYPE_CROSSFADE: Fade in
 * @CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT: Slide in from the left
 * @CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT: Slide in from the right
 * @CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP: Slide in from the bottom
 * @CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN: Slide in from the top
 *
 * These enumeration values describe the possible transitions
 * when the child of a #CustomGtkRevealer widget is shown or hidden.
 */

enum  {
  PROP_0,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_DURATION,
  PROP_SHOW_TRANSITION_DURATION,
  PROP_HIDE_TRANSITION_DURATION,
  PROP_REVEAL_CHILD,
  PROP_CHILD_REVEALED,
  PROP_START_DELAY,
  PROP_SHOW_DELAY,
  PROP_HIDE_DELAY,
  PROP_DELAY,
  LAST_PROP
};

typedef struct {
  CustomGtkRevealerTransitionType transition_type;
  guint transition_duration;
  guint show_transition_duration;
  guint hide_transition_duration;

  GdkWindow* bin_window;
  GdkWindow* view_window;

  gdouble current_pos;
  gdouble source_pos;
  gdouble target_pos;

  gboolean first_start;
  guint start_delay;
  guint delay;
  guint show_delay;
  guint hide_delay;

  guint tick_id;
  CustomGtkProgressTracker tracker;
} CustomGtkRevealerPrivate;

static GParamSpec *props[LAST_PROP] = { NULL, };

static void     custom_gtk_revealer_real_realize                        (GtkWidget     *widget);
static void     custom_gtk_revealer_real_unrealize                      (GtkWidget     *widget);
static void     custom_gtk_revealer_real_add                            (GtkContainer  *widget,
                                                                  GtkWidget     *child);
static void     custom_gtk_revealer_real_size_allocate                  (GtkWidget     *widget,
                                                                  GtkAllocation *allocation);
static void     custom_gtk_revealer_real_map                            (GtkWidget     *widget);
static void     custom_gtk_revealer_real_unmap                          (GtkWidget     *widget);
static gboolean custom_gtk_revealer_real_draw                           (GtkWidget     *widget,
                                                                  cairo_t       *cr);
static void     custom_gtk_revealer_real_get_preferred_height           (GtkWidget     *widget,
                                                                  gint          *minimum_height,
                                                                  gint          *natural_height);
static void     custom_gtk_revealer_real_get_preferred_height_for_width (GtkWidget     *widget,
                                                                  gint           width,
                                                                  gint          *minimum_height,
                                                                  gint          *natural_height);
static void     custom_gtk_revealer_real_get_preferred_width            (GtkWidget     *widget,
                                                                  gint          *minimum_width,
                                                                  gint          *natural_width);
static void     custom_gtk_revealer_real_get_preferred_width_for_height (GtkWidget     *widget,
                                                                  gint           height,
                                                                  gint          *minimum_width,
                                                                  gint          *natural_width);
static guint    custom_custom_gtk_revealer_get_delay_for_cycle(CustomGtkRevealer * revealer);

gboolean custom_settings_get_enable_animations (GtkSettings *settings){
    GValue value = {0, };
    g_value_init (&value, G_TYPE_BOOLEAN);
    g_object_get_property(G_OBJECT(settings),"gtk-enable-animations",&value);
    return g_value_get_boolean (&value);
}

G_DEFINE_TYPE_WITH_PRIVATE (CustomGtkRevealer, custom_gtk_revealer, GTK_TYPE_BIN)

static void
custom_gtk_revealer_get_padding (CustomGtkRevealer *revealer,
                          GtkBorder   *padding)
{
  GtkWidget *widget = GTK_WIDGET (revealer);
  GtkStyleContext *context;
  GtkStateFlags state;

  context = gtk_widget_get_style_context (widget);
  state = gtk_style_context_get_state (context);

  gtk_style_context_get_padding (context, state, padding);
}

static void
custom_gtk_revealer_init (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  priv->transition_type = CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
  priv->transition_duration = 250;
  priv->show_transition_duration = -1;
  priv->hide_transition_duration = -1;
  priv->current_pos = 0.0;
  priv->target_pos = 0.0;
  priv->first_start = TRUE;
  priv->start_delay = -1;
  priv->show_delay = -1;
  priv->hide_delay = -1;
  priv->delay = 0;
  gtk_widget_set_has_window ((GtkWidget*) revealer, TRUE);
}

static void
custom_gtk_revealer_finalize (GObject *obj)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (obj);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  if (priv->tick_id != 0)
    gtk_widget_remove_tick_callback (GTK_WIDGET (revealer), priv->tick_id);
  priv->tick_id = 0;

  G_OBJECT_CLASS (custom_gtk_revealer_parent_class)->finalize (obj);
}

static void
custom_gtk_revealer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (object);

  switch (property_id)
   {
    case PROP_TRANSITION_TYPE:
      g_value_set_enum (value, custom_gtk_revealer_get_transition_type (revealer));
      break;
    case PROP_TRANSITION_DURATION:
      g_value_set_uint (value, custom_gtk_revealer_get_transition_duration (revealer));
      break;
    case PROP_SHOW_TRANSITION_DURATION:
      g_value_set_uint (value, custom_gtk_revealer_get_show_transition_duration (revealer));
      break;
    case PROP_HIDE_TRANSITION_DURATION:
      g_value_set_uint (value, custom_gtk_revealer_get_hide_transition_duration (revealer));
      break;
    case PROP_REVEAL_CHILD:
      g_value_set_boolean (value, custom_gtk_revealer_get_reveal_child (revealer));
      break;
    case PROP_CHILD_REVEALED:
      g_value_set_boolean (value, custom_gtk_revealer_get_child_revealed (revealer));
      break;
    case PROP_START_DELAY:
      g_value_set_uint (value, custom_gtk_revealer_get_start_delay (revealer));
      break;
    case PROP_SHOW_DELAY:
      g_value_set_uint (value, custom_gtk_revealer_get_show_delay (revealer));
      break;
    case PROP_HIDE_DELAY:
      g_value_set_uint (value, custom_gtk_revealer_get_hide_delay (revealer));
      break;
    case PROP_DELAY:
      g_value_set_uint (value, custom_gtk_revealer_get_delay (revealer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
custom_gtk_revealer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (object);

  switch (property_id)
    {
    case PROP_TRANSITION_TYPE:
      custom_gtk_revealer_set_transition_type (revealer, g_value_get_enum (value));
      break;
    case PROP_TRANSITION_DURATION:
      custom_gtk_revealer_set_transition_duration (revealer, g_value_get_uint (value));
      break;
    case PROP_SHOW_TRANSITION_DURATION:
      custom_gtk_revealer_set_show_transition_duration (revealer, g_value_get_uint (value));
      break;
    case PROP_HIDE_TRANSITION_DURATION:
      custom_gtk_revealer_set_hide_transition_duration (revealer, g_value_get_uint (value));
      break;
    case PROP_REVEAL_CHILD:
      custom_gtk_revealer_set_reveal_child (revealer, g_value_get_boolean (value));
      break;
    case PROP_START_DELAY:
      custom_gtk_revealer_set_start_delay (revealer, g_value_get_uint (value));
      break;
    case PROP_SHOW_DELAY:
      custom_gtk_revealer_set_show_delay (revealer, g_value_get_uint (value));
      break;
    case PROP_HIDE_DELAY:
      custom_gtk_revealer_set_hide_delay (revealer, g_value_get_uint (value));
      break;
    case PROP_DELAY:
      custom_gtk_revealer_set_delay (revealer, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
custom_gtk_revealer_class_init (CustomGtkRevealerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = custom_gtk_revealer_get_property;
  object_class->set_property = custom_gtk_revealer_set_property;
  object_class->finalize = custom_gtk_revealer_finalize;

  widget_class->realize = custom_gtk_revealer_real_realize;
  widget_class->unrealize = custom_gtk_revealer_real_unrealize;
  widget_class->size_allocate = custom_gtk_revealer_real_size_allocate;
  widget_class->map = custom_gtk_revealer_real_map;
  widget_class->unmap = custom_gtk_revealer_real_unmap;
  widget_class->draw = custom_gtk_revealer_real_draw;
  widget_class->get_preferred_height = custom_gtk_revealer_real_get_preferred_height;
  widget_class->get_preferred_height_for_width = custom_gtk_revealer_real_get_preferred_height_for_width;
  widget_class->get_preferred_width = custom_gtk_revealer_real_get_preferred_width;
  widget_class->get_preferred_width_for_height = custom_gtk_revealer_real_get_preferred_width_for_height;

  container_class->add = custom_gtk_revealer_real_add;

  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type",
                       P_("Transition type"),
                       P_("The type of animation used to transition"),
                       GTK_TYPE_REVEALER_TRANSITION_TYPE,
                       CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration",
                       P_("Transition duration"),
                       P_("The animation duration, in milliseconds"),
                       0, G_MAXUINT, 250,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SHOW_TRANSITION_DURATION] =
    g_param_spec_uint ("show-transition-duration",
                       P_("Show transition duration"),
                       P_("The animation duration for showing the widget, in milliseconds"),
                       0, G_MAXUINT, -1,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_HIDE_TRANSITION_DURATION] =
    g_param_spec_uint ("hide-transition-duration",
                       P_("Hide transition duration"),
                       P_("The animation duration for hiding the widget, in milliseconds"),
                       0, G_MAXUINT, -1,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_REVEAL_CHILD] =
    g_param_spec_boolean ("reveal-child",
                          P_("Reveal Child"),
                          P_("Whether the container should reveal the child"),
                          FALSE,
                          CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_CHILD_REVEALED] =
    g_param_spec_boolean ("child-revealed",
                          P_("Child Revealed"),
                          P_("Whether the child is revealed and the animation target reached"),
                          FALSE,
                          G_PARAM_READABLE);

  props[PROP_START_DELAY] =
    g_param_spec_uint ("start-delay",
                          P_("Animation start delay"),
                          P_("Time to wait before starting the animation"),
                       0, G_MAXUINT, -1,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SHOW_DELAY] =
    g_param_spec_uint ("show-delay",
                          P_("Animation show delay"),
                          P_("Time to wait before showing the widget"),
                       0, G_MAXUINT, -1,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_HIDE_DELAY] =
    g_param_spec_uint ("hide-delay",
                          P_("Animation hide delay"),
                          P_("Time to wait before hiding the widget"),
                       0, G_MAXUINT, -1,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_DELAY] =
    g_param_spec_uint ("delay",
                          P_("Animation delay"),
                          P_("Time to wait before each animation cycle"),
                       0, G_MAXUINT, 0,
                       CUSTOM_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "revealer");
}

/**
 * custom_gtk_revealer_new:
 *
 * Creates a new #CustomGtkRevealer.
 *
 * Returns: a newly created #CustomGtkRevealer
 *
 * Since: 3.10
 */
GtkWidget *
custom_gtk_revealer_new (void)
{
  return g_object_new (CUSTOM_GTK_TYPE_REVEALER, NULL);
}

static CustomGtkRevealerTransitionType
effective_transition (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  if (gtk_widget_get_direction (GTK_WIDGET (revealer)) == GTK_TEXT_DIR_RTL)
    {
      if (priv->transition_type == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT)
        return CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
      else if (priv->transition_type == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        return CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
    }

  return priv->transition_type;
}

static void
custom_gtk_revealer_get_child_allocation (CustomGtkRevealer   *revealer,
                                   GtkAllocation *allocation,
                                   GtkAllocation *child_allocation)
{
  GtkWidget *child;
  CustomGtkRevealerTransitionType transition;
  GtkBorder padding;
  gint vertical_padding, horizontal_padding;

  g_return_if_fail (revealer != NULL);
  g_return_if_fail (allocation != NULL);

  /* See explanation on custom_gtk_revealer_real_size_allocate */
  custom_gtk_revealer_get_padding (revealer, &padding);
  vertical_padding = padding.top + padding.bottom;
  horizontal_padding = padding.left + padding.right;

  child_allocation->x = 0;
  child_allocation->y = 0;
  child_allocation->width = 0;
  child_allocation->height = 0;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL && gtk_widget_get_visible (child))
    {
      transition = effective_transition (revealer);
      if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
          transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        gtk_widget_get_preferred_width_for_height (child, MAX (0, allocation->height - vertical_padding), NULL,
                                                   &child_allocation->width);
      else
        gtk_widget_get_preferred_height_for_width (child, MAX (0, allocation->width - horizontal_padding), NULL,
                                                   &child_allocation->height);
    }

  child_allocation->width = MAX (child_allocation->width, allocation->width - horizontal_padding);
  child_allocation->height = MAX (child_allocation->height, allocation->height - vertical_padding);
}

static void
custom_gtk_revealer_real_realize (GtkWidget *widget)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  GtkAllocation allocation;
  GdkWindowAttr attributes = { 0 };
  GdkWindowAttributesType attributes_mask;
  GtkAllocation child_allocation;
  GtkWidget *child;
  CustomGtkRevealerTransitionType transition;
  GtkBorder padding;

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask =
    gtk_widget_get_events (widget);
  attributes_mask = (GDK_WA_X | GDK_WA_Y) | GDK_WA_VISUAL;

  priv->view_window =
    gdk_window_new (gtk_widget_get_parent_window ((GtkWidget*) revealer),
                    &attributes, attributes_mask);
  gtk_widget_set_window (widget, priv->view_window);
  gtk_widget_register_window (widget, priv->view_window);

  custom_gtk_revealer_get_child_allocation (revealer, &allocation, &child_allocation);

  custom_gtk_revealer_get_padding (revealer, &padding);
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = child_allocation.width;
  attributes.height = child_allocation.height;

  /* See explanation on custom_gtk_revealer_real_size_allocate */
  transition = effective_transition (revealer);
  if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
    {
      attributes.y = allocation.height - child_allocation.height - padding.bottom;
      attributes.x = padding.left;
    }
  else if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
    {
      attributes.y = padding.top;
      attributes.x = allocation.width - child_allocation.width - padding.right;
    }
 else
   {
     attributes.y = padding.top;
     attributes.x = padding.left;
   }

  priv->bin_window =
    gdk_window_new (priv->view_window, &attributes, attributes_mask);
  gtk_widget_register_window (widget, priv->bin_window);

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL)
    gtk_widget_set_parent_window (child, priv->bin_window);

  gdk_window_show (priv->bin_window);
}

static void
custom_gtk_revealer_real_unrealize (GtkWidget *widget)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  gtk_widget_unregister_window (widget, priv->bin_window);
  gdk_window_destroy (priv->bin_window);
  priv->view_window = NULL;

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->unrealize (widget);
}

static void
custom_gtk_revealer_real_add (GtkContainer *container,
                       GtkWidget    *child)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (container);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (child != NULL);

  gtk_widget_set_parent_window (child, priv->bin_window);
  gtk_widget_set_child_visible (child, priv->current_pos != 0.0);

  GTK_CONTAINER_CLASS (custom_gtk_revealer_parent_class)->add (container, child);
}

static void
custom_gtk_revealer_real_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  GtkAllocation child_allocation;
  GtkWidget *child;
  gboolean window_visible;
  int bin_x, bin_y;
  CustomGtkRevealerTransitionType transition;
  GtkBorder padding;

  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);
  custom_gtk_revealer_get_child_allocation (revealer, allocation, &child_allocation);

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL && gtk_widget_get_visible (child))
    gtk_widget_size_allocate (child, &child_allocation);

  if (gtk_widget_get_realized (widget))
    {
      if (gtk_widget_get_mapped (widget))
        {
          window_visible = allocation->width > 0 && allocation->height > 0;

          if (!window_visible && gdk_window_is_visible (priv->view_window))
            gdk_window_hide (priv->view_window);

          if (window_visible && !gdk_window_is_visible (priv->view_window))
            gdk_window_show (priv->view_window);
        }

      /* The view window will follow the revealer allocation, which is modified
       * along the animation */
      gdk_window_move_resize (priv->view_window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      custom_gtk_revealer_get_padding (revealer, &padding);
      bin_x = 0;
      bin_y = 0;

      transition = effective_transition (revealer);
      /* The child allocation is fixed (it is not modified by the animation),
       * and it's origin is relative to the bin_window.
       * The bin_window has the same allocation as the child, and then the bin_window
       * deals with the relative positioning with respect to the revealer taking
       * into account the paddings of the revealer.
       *
       * For most of transitions, the bin_window moves along with the revealer,
       * as its allocation changes.
       * However for CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
       * we need to first move the bin_window upwards and then slide it down in
       * the revealer.
       * Otherwise the child would appear as static and the revealer will allocate
       * following the animation, clipping the child.
       * To calculate the correct y position for this case:
       * allocation->height - child_allocation.height is the relative position
       * towards the revealer taking into account the animation progress with
       * both vertical paddings added, therefore we need to substract the part
       * that we don't want to take into account for the y position, which
       * in this case is the bottom padding.
       *
       * The same special treatment is needed for CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT.
       */
      if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
        {
          bin_y = allocation->height - child_allocation.height - padding.bottom;
          bin_x = padding.left;
        }
      else if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        {
          bin_y = padding.top;
          bin_x = allocation->width - child_allocation.width - padding.right;
        }
     else
       {
         bin_x = padding.left;
         bin_y = padding.top;
       }

      gdk_window_move_resize (priv->bin_window,
                              bin_x, bin_y,
                              child_allocation.width, child_allocation.height);
    }
}

static void
custom_gtk_revealer_set_position (CustomGtkRevealer *revealer,
                           gdouble      pos)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  gboolean new_visible;
  GtkWidget *child;
  CustomGtkRevealerTransitionType transition;

  priv->current_pos = pos;

  /* We check target_pos here too, because we want to ensure we set
   * child_visible immediately when starting a reveal operation
   * otherwise the child widgets will not be properly realized
   * after the reveal returns.
   */
  new_visible = priv->current_pos != 0.0 || priv->target_pos != 0.0;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL &&
      new_visible != gtk_widget_get_child_visible (child))
    gtk_widget_set_child_visible (child, new_visible);

  transition = effective_transition (revealer);
  if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_CROSSFADE)
    {
      gtk_widget_set_opacity (GTK_WIDGET (revealer), priv->current_pos);
      gtk_widget_queue_draw (GTK_WIDGET (revealer));
    }
  else
    {
      gtk_widget_queue_resize (GTK_WIDGET (revealer));
    }

  if (priv->current_pos == priv->target_pos)
    g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_CHILD_REVEALED]);
}

static gboolean
custom_gtk_revealer_animate_cb (GtkWidget     *widget,
                         GdkFrameClock *frame_clock,
                         gpointer       user_data)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  gdouble ease;

  custom_gtk_progress_tracker_advance_frame (&priv->tracker,
                                      gdk_frame_clock_get_frame_time (frame_clock));
  ease = custom_gtk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);
  custom_gtk_revealer_set_position (revealer,
                             priv->source_pos + (ease * (priv->target_pos - priv->source_pos)));

  if (custom_gtk_progress_tracker_get_state (&priv->tracker) == CUSTOM_GTK_PROGRESS_STATE_AFTER)
    {
      priv->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static guint
custom_custom_gtk_revealer_get_delay_for_cycle(CustomGtkRevealer * revealer){
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  
  guint delay = priv->delay;
  if(priv->first_start && priv->start_delay != -1U){
    priv->first_start = FALSE;
    delay = priv->start_delay;
  } else if(priv->current_pos == 0 && priv->show_delay != -1U){
    delay = priv->show_delay;
  } else if(priv->current_pos == 1 && priv->hide_delay != -1U){
    delay = priv->hide_delay;
  }

  return delay;
}

static guint
custom_custom_gtk_revealer_get_duration_for_cycle(CustomGtkRevealer * revealer){
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  
  guint duration = priv->transition_duration;
  if(priv->current_pos == 0 && priv->show_transition_duration != -1U){
    duration = priv->show_transition_duration;
  } else if(priv->current_pos == 1 && priv->hide_transition_duration != -1U){
    duration = priv->hide_transition_duration;
  }

  return duration;
}

static void
custom_gtk_revealer_start_animation (CustomGtkRevealer *revealer,
                              gdouble      target)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  GtkWidget *widget = GTK_WIDGET (revealer);
  CustomGtkRevealerTransitionType transition;

  if (priv->target_pos == target)
    return;

  priv->target_pos = target;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_REVEAL_CHILD]);

  transition = effective_transition (revealer);
  if (gtk_widget_get_mapped (widget) &&
      priv->transition_duration != 0 &&
      transition != CUSTOM_GTK_REVEALER_TRANSITION_TYPE_NONE &&
      custom_settings_get_enable_animations (gtk_widget_get_settings (widget)))
    {
      priv->source_pos = priv->current_pos;
      if (priv->tick_id == 0)
        priv->tick_id =
          gtk_widget_add_tick_callback (widget, custom_gtk_revealer_animate_cb, revealer, NULL);
      custom_gtk_progress_tracker_start (&priv->tracker,
                                  custom_custom_gtk_revealer_get_duration_for_cycle(revealer) * 1000,
                                  custom_custom_gtk_revealer_get_delay_for_cycle(revealer) * 1000,
                                  1.0);
    }
  else
    {
      custom_gtk_revealer_set_position (revealer, target);
    }
}

static void
custom_gtk_revealer_stop_animation (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  if (priv->current_pos != priv->target_pos)
    custom_gtk_revealer_set_position (revealer, priv->target_pos);
  if (priv->tick_id != 0)
    {
      gtk_widget_remove_tick_callback (GTK_WIDGET (revealer), priv->tick_id);
      priv->tick_id = 0;
    }
}

static void
custom_gtk_revealer_real_map (GtkWidget *widget)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  GtkAllocation allocation;

  if (!gtk_widget_get_mapped (widget))
    {
      gtk_widget_get_allocation (widget, &allocation);

      if (allocation.width > 0 && allocation.height > 0)
        gdk_window_show (priv->view_window);
    }

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->map (widget);
}

static void
custom_gtk_revealer_real_unmap (GtkWidget *widget)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->unmap (widget);

  custom_gtk_revealer_stop_animation (revealer);
}

static gboolean
custom_gtk_revealer_real_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  if (gtk_cairo_should_draw_window (cr, priv->bin_window))
    GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->draw (widget, cr);

  return GDK_EVENT_PROPAGATE;
}

/**
 * custom_gtk_revealer_set_reveal_child:
 * @revealer: a #CustomGtkRevealer
 * @reveal_child: %TRUE to reveal the child
 *
 * Tells the #CustomGtkRevealer to reveal or conceal its child.
 *
 * The transition will be animated with the current
 * transition type of @revealer.
 *
 * Since: 3.10
 */
void
custom_gtk_revealer_set_reveal_child (CustomGtkRevealer *revealer,
                               gboolean     reveal_child)
{
  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  if (reveal_child)
    custom_gtk_revealer_start_animation (revealer, 1.0);
  else
    custom_gtk_revealer_start_animation (revealer, 0.0);
}

/**
 * custom_gtk_revealer_get_reveal_child:
 * @revealer: a #CustomGtkRevealer
 *
 * Returns whether the child is currently
 * revealed. See custom_gtk_revealer_set_reveal_child().
 *
 * This function returns %TRUE as soon as the transition
 * is to the revealed state is started. To learn whether
 * the child is fully revealed (ie the transition is completed),
 * use custom_gtk_revealer_get_child_revealed().
 *
 * Returns: %TRUE if the child is revealed.
 *
 * Since: 3.10
 */
gboolean
custom_gtk_revealer_get_reveal_child (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), FALSE);

  return priv->target_pos != 0.0;
}

/**
 * custom_gtk_revealer_get_child_revealed:
 * @revealer: a #CustomGtkRevealer
 *
 * Returns whether the child is fully revealed, in other words whether
 * the transition to the revealed state is completed.
 *
 * Returns: %TRUE if the child is fully revealed
 *
 * Since: 3.10
 */
gboolean
custom_gtk_revealer_get_child_revealed (CustomGtkRevealer *revealer)
{
  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), FALSE);

  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  gboolean animation_finished = (priv->target_pos == priv->current_pos);
  gboolean reveal_child = custom_gtk_revealer_get_reveal_child (revealer);

  if (animation_finished)
    return reveal_child;
  else
    return !reveal_child;
}

void custom_gtk_revealer_restart(CustomGtkRevealer *revealer){
  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  priv->first_start = TRUE;

  //Reset tracker state
  memset(&priv->tracker,0,sizeof(priv->tracker));

  //Reset child state without animation
  GtkWidget * child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL) gtk_widget_set_child_visible (child, FALSE);
  priv->current_pos = 0.0;
  priv->target_pos = 0.0;
}

/**
 * custom_gtk_revealer_set_start_delay:
 * @revealer: a #GtkRevealer
 * @start_delay: Greater than 0 to set animation start delay
 *
 * Tells the #GtkRevealer to wait before starting the animation
 *
 * This setting overrides the delay attribute only for the first cycle
 *
 * Since: 3.24
 */
void
custom_gtk_revealer_set_start_delay (CustomGtkRevealer *revealer,
                               guint     start_delay)
{
  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  priv->start_delay = start_delay;
}

guint custom_gtk_revealer_get_start_delay (CustomGtkRevealer *revealer){
  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  return priv->start_delay;
}

/**
 * custom_gtk_revealer_set_delay:
 * @revealer: a #GtkRevealer
 * @delay: Greater than 0 to set animation before every cycle
 *
 * Tells the #GtkRevealer to wait before starting each animation cycle
 *
 * Since: 3.24
 */
void
custom_gtk_revealer_set_delay (CustomGtkRevealer *revealer,
                               guint     delay)
{
  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  priv->delay = delay;
}

guint custom_gtk_revealer_get_delay (CustomGtkRevealer *revealer){
  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  return priv->delay;
}

/**
 * custom_gtk_revealer_set_show_delay:
 * @revealer: a #GtkRevealer
 * @show_delay: Greater than 0 to set animation delay before showing the widget
 *
 * Tells the #GtkRevealer to wait before showing the widget
 *
 * Since: 3.24
 */
void
custom_gtk_revealer_set_show_delay (CustomGtkRevealer *revealer,
                               guint     show_delay)
{
  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  priv->show_delay = show_delay;
}

guint custom_gtk_revealer_get_show_delay (CustomGtkRevealer *revealer){
  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  return priv->show_delay;
}

/**
 * custom_gtk_revealer_set_hide_delay:
 * @revealer: a #GtkRevealer
 * @hide_delay: Greater than 0 to set animation delay before hiding the widget
 *
 * Tells the #GtkRevealer to wait before hiding the widget
 *
 * Since: 3.24
 */
void
custom_gtk_revealer_set_hide_delay (CustomGtkRevealer *revealer,
                               guint     hide_delay)
{
  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  priv->hide_delay = hide_delay;
}

guint custom_gtk_revealer_get_hide_delay (CustomGtkRevealer *revealer){
  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  return priv->hide_delay;
}


/* These all report only the natural size, ignoring the minimal size,
 * because its not really possible to allocate the right size during
 * animation if the child size can change (without the child
 * re-arranging itself during the animation).
 */

static void
set_height_with_paddings (CustomGtkRevealer *revealer,
                          gint         preferred_minimum_height,
                          gint         preferred_natural_height,
                          gint        *minimum_height_out,
                          gint        *natural_height_out)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  gint minimum_height;
  gint natural_height;
  CustomGtkRevealerTransitionType transition;
  GtkBorder padding;
  gint vertical_padding;

  custom_gtk_revealer_get_padding (revealer, &padding);
  vertical_padding = padding.top + padding.bottom;
  minimum_height = preferred_minimum_height + vertical_padding;
  natural_height = preferred_natural_height + vertical_padding;

  transition = effective_transition (revealer);
  if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP ||
      transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
    {
      /* Padding are included in the animation */
      minimum_height = round (minimum_height * priv->current_pos);
      natural_height = round (natural_height * priv->current_pos);
    }

  *minimum_height_out = MIN (minimum_height, natural_height);
  *natural_height_out = natural_height;
}

static void
custom_gtk_revealer_real_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum_height_out,
                                        gint      *natural_height_out)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  gint minimum_height;
  gint natural_height;

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->get_preferred_height (widget, &minimum_height, &natural_height);

  set_height_with_paddings (revealer, minimum_height, natural_height,
                            minimum_height_out, natural_height_out);
}

static void
custom_gtk_revealer_real_get_preferred_height_for_width (GtkWidget *widget,
                                                  gint       width,
                                                  gint      *minimum_height_out,
                                                  gint      *natural_height_out)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  gint minimum_height;
  gint natural_height;

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->get_preferred_height_for_width (widget, width, &minimum_height, &natural_height);

  set_height_with_paddings (revealer, minimum_height, natural_height,
                            minimum_height_out, natural_height_out);
}

static void
set_width_with_paddings (CustomGtkRevealer *revealer,
                         gint         preferred_minimum_width,
                         gint         preferred_natural_width,
                         gint        *minimum_width_out,
                         gint        *natural_width_out)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);
  gint minimum_width;
  gint natural_width;
  CustomGtkRevealerTransitionType transition;
  GtkBorder padding;
  gint horizontal_padding;

  custom_gtk_revealer_get_padding (revealer, &padding);
  horizontal_padding = padding.left + padding.right;
  minimum_width = preferred_minimum_width + horizontal_padding;
  natural_width = preferred_natural_width + horizontal_padding;

  transition = effective_transition (revealer);
  if (transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
      transition == CUSTOM_GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
    {
      /* Paddings are included in the animation */
      minimum_width = round (minimum_width * priv->current_pos);
      natural_width = round (natural_width * priv->current_pos);
    }

  *minimum_width_out = MIN (minimum_width, natural_width);
  *natural_width_out = natural_width;
}

static void
custom_gtk_revealer_real_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum_width_out,
                                       gint      *natural_width_out)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  gint minimum_width;
  gint natural_width;

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->get_preferred_width (widget, &minimum_width, &natural_width);
  set_width_with_paddings (revealer, minimum_width, natural_width,
                           minimum_width_out, natural_width_out);
}

static void
custom_gtk_revealer_real_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width_out,
                                                  gint      *natural_width_out)
{
  CustomGtkRevealer *revealer = CUSTOM_GTK_REVEALER (widget);
  gint minimum_width;
  gint natural_width;

  GTK_WIDGET_CLASS (custom_gtk_revealer_parent_class)->get_preferred_width_for_height (widget, height, &minimum_width, &natural_width);

  set_width_with_paddings (revealer, minimum_width, natural_width,
                           minimum_width_out, natural_width_out);
}

/**
 * custom_gtk_revealer_get_transition_duration:
 * @revealer: a #CustomGtkRevealer
 *
 * Returns the amount of time (in milliseconds) that
 * transitions will take.
 *
 * Returns: the transition duration
 *
 * Since: 3.10
 */
guint
custom_gtk_revealer_get_transition_duration (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);

  return priv->transition_duration;
}

/**
 * custom_gtk_revealer_set_transition_duration:
 * @revealer: a #CustomGtkRevealer
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions will take.
 *
 * Since: 3.10
 */
void
custom_gtk_revealer_set_transition_duration (CustomGtkRevealer *revealer,
                                      guint        value)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  if (priv->transition_duration == value)
    return;

  if(value > 0){
    priv->transition_duration = value;
  } else {
    priv->transition_duration = 1; //Fail safe.
  }
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_DURATION]);
}

/**
 * custom_gtk_revealer_get_show_transition_duration:
 * @revealer: a #CustomGtkRevealer
 *
 * Returns the amount of time (in milliseconds) that
 * showing transitions will take.
 *
 * Returns: the transition duration
 *
 * Since: 3.10
 */
guint
custom_gtk_revealer_get_show_transition_duration (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);

  return priv->show_transition_duration;
}

/**
 * custom_gtk_revealer_set_show_transition_duration:
 * @revealer: a #CustomGtkRevealer
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that showing transitions will take.
 *
 * Since: 3.10
 */
void
custom_gtk_revealer_set_show_transition_duration (CustomGtkRevealer *revealer,
                                      guint        value)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  if (priv->show_transition_duration == value)
    return;

  priv->show_transition_duration = value;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_SHOW_TRANSITION_DURATION]);
}

/**
 * custom_gtk_revealer_get_hide_transition_duration:
 * @revealer: a #CustomGtkRevealer
 *
 * Returns the amount of time (in milliseconds) that
 * hiding transitions will take.
 *
 * Returns: the transition duration
 *
 * Since: 3.10
 */
guint
custom_gtk_revealer_get_hide_transition_duration (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), 0);

  return priv->hide_transition_duration;
}

/**
 * custom_gtk_revealer_set_hide_transition_duration:
 * @revealer: a #CustomGtkRevealer
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that hiding transitions will take.
 *
 * Since: 3.10
 */
void
custom_gtk_revealer_set_hide_transition_duration (CustomGtkRevealer *revealer,
                                      guint        value)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  if (priv->hide_transition_duration == value)
    return;

  priv->hide_transition_duration = value;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_HIDE_TRANSITION_DURATION]);
}

/**
 * custom_gtk_revealer_get_transition_type:
 * @revealer: a #CustomGtkRevealer
 *
 * Gets the type of animation that will be used
 * for transitions in @revealer.
 *
 * Returns: the current transition type of @revealer
 *
 * Since: 3.10
 */
CustomGtkRevealerTransitionType
custom_gtk_revealer_get_transition_type (CustomGtkRevealer *revealer)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CUSTOM_IS_GTK_REVEALER (revealer), CUSTOM_GTK_REVEALER_TRANSITION_TYPE_NONE);

  return priv->transition_type;
}

/**
 * custom_gtk_revealer_set_transition_type:
 * @revealer: a #CustomGtkRevealer
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for
 * transitions in @revealer. Available types include
 * various kinds of fades and slides.
 *
 * Since: 3.10
 */
void
custom_gtk_revealer_set_transition_type (CustomGtkRevealer               *revealer,
                                  CustomGtkRevealerTransitionType  transition)
{
  CustomGtkRevealerPrivate *priv = custom_gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (CUSTOM_IS_GTK_REVEALER (revealer));

  if (priv->transition_type == transition)
    return;

  priv->transition_type = transition;
  gtk_widget_queue_resize (GTK_WIDGET (revealer));
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_TYPE]);
}
