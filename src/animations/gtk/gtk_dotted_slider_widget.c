#include "gtk_dotted_slider_widget.h"
#include "custom_gtk_revealer.h"
#include <gtk/gtk.h>
#include "clogger.h"

#define GTK_DOTTED_SLIDER_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

const char * GTK_DOTTED_SLIDER_DEFAULT_DOT_CSS = "* {\n" \
" border-radius:19px;\n" \
" min-height: 19px;\n" \
" min-width: 19px;\n" \
" background-color: currentColor;\n" \
"}";

enum  {
  PROP_0,
  PROP_ITEM_COUNT,
  PROP_ANIMATION_TIME,
  LAST_PROP
};

typedef struct {
    GList * dots;
    gdouble animation_time;
    gint item_count;
} GtkDottedSliderPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GtkDottedSlider, gtk_dotted_slider, GTK_TYPE_BOX)

static GParamSpec *props[LAST_PROP] = { NULL, };

static void 
gtk_dotted_slider_init (GtkDottedSlider *slider)
{
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (slider);
  priv->dots = NULL;
  priv->animation_time = 2.0;
  priv->item_count = 5;
  C_TRAIL("Created dotted slider");
}

gint
gtk_dotted_slider_get_item_count (GtkDottedSlider *slider)
{
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (slider);

  g_return_val_if_fail (GTK_IS_DOTTED_SLIDER (slider), 0);

  return priv->item_count;
}

static void gtk_dotted_slider_change_direction (CustomGtkRevealer *revealer){
    C_TRAIL("gtk_dotted_slider_change_direction %p", (void*) revealer);
    gboolean revealed = custom_gtk_revealer_get_reveal_child(CUSTOM_GTK_REVEALER(revealer));
    if (gtk_widget_get_mapped (GTK_WIDGET (revealer))){
        custom_gtk_revealer_set_reveal_child(CUSTOM_GTK_REVEALER(revealer),!revealed);
    }
}

void gtk_dotted_slider_map_event (GtkWidget* self, gpointer user_data){
    C_TRAIL("gtk_dotted_slider mapping dot %p", (void*) self);
    custom_gtk_revealer_restart(CUSTOM_GTK_REVEALER(self));
    custom_gtk_revealer_set_reveal_child(CUSTOM_GTK_REVEALER(self),TRUE);
}



void gtk_dotted_slider_refresh_items(GtkDottedSlider *slider){
  C_TRAIL("gtk_dotted_slider_refresh_items");
  GtkCssProvider * cssProvider;
  GtkStyleContext * context;
  double delay;
  int i = 0;
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (slider);

  //Reset existing animation timers and states
  GList *node_itr = priv->dots;
  while (node_itr != NULL)
  {
    CustomGtkRevealer * dot = (CustomGtkRevealer*) node_itr->data;
    if(i >= priv->item_count){
      gtk_widget_destroy(GTK_WIDGET(dot));
      priv->dots = g_list_remove_link(priv->dots, node_itr);
      g_list_free_1(node_itr);
      i++;
      node_itr = g_list_next(node_itr);
      continue;
    }
    
    delay = (double) priv->animation_time / (double)priv->item_count * (i+1) - priv->animation_time / (double)priv->item_count;
    custom_gtk_revealer_set_start_delay(dot,delay*1000);
    custom_gtk_revealer_set_hide_transition_duration(dot,priv->animation_time*1000);
    custom_gtk_revealer_restart(dot);
    node_itr = g_list_next(node_itr);
    i++;
  }

  //Create missing dots
  cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(cssProvider,GTK_DOTTED_SLIDER_DEFAULT_DOT_CSS,-1,NULL); 

  for(;i < priv->item_count;i++){
      delay = (double) priv->animation_time / (double)priv->item_count * (i+1) - priv->animation_time / (double)priv->item_count;

      GtkWidget * revealer = custom_gtk_revealer_new();
      custom_gtk_revealer_set_transition_type(CUSTOM_GTK_REVEALER(revealer),CUSTOM_GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
      custom_gtk_revealer_set_transition_duration(CUSTOM_GTK_REVEALER(revealer),0);
      custom_gtk_revealer_set_hide_transition_duration(CUSTOM_GTK_REVEALER(revealer),priv->animation_time*1000);
      custom_gtk_revealer_set_start_delay(CUSTOM_GTK_REVEALER(revealer),delay*1000);

      g_signal_connect (revealer, "notify::child-revealed", G_CALLBACK (gtk_dotted_slider_change_direction), NULL);
      g_signal_connect (revealer, "map", G_CALLBACK (gtk_dotted_slider_map_event), NULL);

      GtkWidget * widget = gtk_label_new("");

      context = gtk_widget_get_style_context(widget);
      gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      
      gtk_container_add(GTK_CONTAINER(revealer),widget);

      gtk_box_pack_start(GTK_BOX(slider), revealer, TRUE, FALSE, 0);

      gtk_widget_show_all(GTK_WIDGET(revealer));
      priv->dots = g_list_append(priv->dots, revealer);
  }

  g_object_unref (cssProvider); 
  C_TRAIL("gtk_dotted_slider_refresh_items - done");
}

void
gtk_dotted_slider_set_item_count (GtkDottedSlider *revealer,
                                      gint        value)
{
  C_TRAIL("gtk_dotted_slider_set_item_count");
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (revealer);

  g_return_if_fail (GTK_IS_DOTTED_SLIDER (revealer));

  if (priv->item_count == value)
    return;

  priv->item_count = value;

  if(gtk_widget_get_mapped(GTK_WIDGET(revealer)))
    gtk_dotted_slider_refresh_items(revealer);

  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_ITEM_COUNT]);
}

gdouble
gtk_dotted_slider_get_animation_time (GtkDottedSlider *revealer)
{
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (revealer);

  g_return_val_if_fail (GTK_IS_DOTTED_SLIDER (revealer), 0);

  return priv->animation_time;
}

void
gtk_dotted_slider_set_animation_time (GtkDottedSlider *revealer,
                                      gdouble        value)
{
  C_TRAIL("gtk_dotted_slider_set_animation_time");
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (revealer);

  g_return_if_fail (GTK_IS_DOTTED_SLIDER (revealer));

  if (priv->animation_time == value)
    return;

  priv->animation_time = value;
  if(gtk_widget_get_mapped(GTK_WIDGET(revealer)))
    gtk_dotted_slider_refresh_items(revealer);

  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_ANIMATION_TIME]);
}

static void
gtk_dotted_slider_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GtkDottedSlider *revealer = GTK_DOTTED_SLIDER (object);

  switch (property_id)
   {
    case PROP_ITEM_COUNT:
      g_value_set_int (value, gtk_dotted_slider_get_item_count (revealer));
      break;
    case PROP_ANIMATION_TIME:
      g_value_set_int (value, gtk_dotted_slider_get_animation_time (revealer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_dotted_slider_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GtkDottedSlider *revealer = GTK_DOTTED_SLIDER (object);

  switch (property_id)
    {
    case PROP_ITEM_COUNT:
      gtk_dotted_slider_set_item_count (revealer, g_value_get_int (value));
      break;
    case PROP_ANIMATION_TIME:
      gtk_dotted_slider_set_animation_time (revealer, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_dotted_slider_dispose(GObject * gobject){
  GtkDottedSliderPrivate *priv = gtk_dotted_slider_get_instance_private (GTK_DOTTED_SLIDER(gobject));

  if(priv->dots){
    g_list_free (priv->dots);
    priv->dots = NULL;
  }

  G_OBJECT_CLASS (gtk_dotted_slider_parent_class)->dispose (gobject);
}

static void
gtk_dotted_slider_map(GtkWidget * widget){
  gtk_dotted_slider_refresh_items(GTK_DOTTED_SLIDER(widget));
  GTK_WIDGET_CLASS (gtk_dotted_slider_parent_class)->map (widget);
}

static void 
gtk_dotted_slider_class_init (GtkDottedSliderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    widget_class->map = gtk_dotted_slider_map;
    object_class->dispose = gtk_dotted_slider_dispose;
    object_class->get_property = gtk_dotted_slider_get_property;
    object_class->set_property = gtk_dotted_slider_set_property;

    props[PROP_ITEM_COUNT] =
        g_param_spec_int ("item-count",
                        "Item count",
                        "The number of dots in the slider",
                        0, G_MAXINT, 10,
                        GTK_DOTTED_SLIDER_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

    props[PROP_ANIMATION_TIME] =
        g_param_spec_int ("animation-time",
                        "Transition duration",
                        "The animation duration, in milliseconds",
                        0, G_MAXINT, 1,
                        GTK_DOTTED_SLIDER_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties (object_class, LAST_PROP, props);

    gtk_widget_class_set_css_name (widget_class, "dottedslider");
    C_TRAIL("gtk_dotted_slider type initialized");

}

GtkWidget*     gtk_dotted_slider_new             (GtkOrientation orientation, int spacing, int item_count, int animation_time){
    return g_object_new (GTK_TYPE_DOTTED_SLIDER,
                       "orientation", orientation,
                       "spacing", spacing,
                       "animation-time", animation_time,
                       "item-count", item_count,
                       NULL);
}