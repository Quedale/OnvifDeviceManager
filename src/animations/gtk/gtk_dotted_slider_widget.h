#ifndef GTK_DOTTED_SLIDDER_WIDGET_H_ 
#define GTK_DOTTED_SLIDDER_WIDGET_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GtkDottedSlider       GtkDottedSlider;
typedef struct _GtkDottedSliderClass  GtkDottedSliderClass;

#define GTK_TYPE_DOTTED_SLIDER (gtk_dotted_slider_get_type ())
#define GTK_DOTTED_SLIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_DOTTED_SLIDER, GtkDottedSlider))//(obj)          GTK_CHECK_CAST (obj, gtk_dotted_slider_get_type (), GtkDottedSlider)
#define GTK_DOTTED_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_DOTTED_SLIDER, GtkDottedSliderClass)) //(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_dotted_slider_get_type (), GtkDottedSliderClass)
#define GTK_IS_DOTTED_SLIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_DOTTED_SLIDER)) //      GTK_CHECK_TYPE (obj, gtk_dotted_slider_get_type ())
#define GTK_IS_DOTTED_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DOTTED_SLIDER))
#define GTK_DOTTED_SLIDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DOTTED_SLIDER, GtkDottedSliderClass))

struct _GtkDottedSlider
{
    GtkBox vbox;
};

struct _GtkDottedSliderClass
{
    GtkBoxClass parent_class;

};

GType          gtk_dotted_slider_get_type        (void);
GtkWidget*     gtk_dotted_slider_new             (GtkOrientation orientation, int spacing, int item_count, int animation_time);
gint           gtk_dotted_slider_get_item_count  (GtkDottedSlider *slider);
void           gtk_dotted_slider_set_item_count  (GtkDottedSlider *revealer, gint value);
gdouble        gtk_dotted_slider_get_animation_time (GtkDottedSlider *revealer);
void           gtk_dotted_slider_set_animation_time (GtkDottedSlider *revealer, gdouble value);

G_END_DECLS

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif