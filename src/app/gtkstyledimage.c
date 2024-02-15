#include "gtkstyledimage.h"
#include "gui_utils.h"
#include "clogger.h"


typedef struct {
    unsigned char * data_start;
    unsigned int data_size;
    GdkRGBA current_color;
} GtkStyledImagePrivate;


struct GSIData {
    GdkPixbuf * pixbuf;
    int height;
    int width;
};

G_DEFINE_TYPE_WITH_PRIVATE(GtkStyledImage, GtkStyledImage_, GTK_TYPE_BINARYIMAGE)

GdkPixbuf * pixbuf_replace_color(GdkPixbuf * src, GdkRGBA * color){
    guchar * pixels = gdk_pixbuf_get_pixels(src);
    int width = gdk_pixbuf_get_width (src);
	int height = gdk_pixbuf_get_height (src);
    int stride = gdk_pixbuf_get_rowstride (src);
    int channels = gdk_pixbuf_get_n_channels (src);
    int x, y;

    for (x = 0; x < width; x++) {
        for (y = 0; y <  height ; y++) {
            guchar *p = pixels + y * stride + x * channels;
            p[0] = color->red * 255;
            p[1] = color->green * 255;
            p[2] = color->blue * 255;
        }
    }

    GdkPixbuf *  ret = gdk_pixbuf_new_from_data(pixels,
        gdk_pixbuf_get_colorspace(src),
        gdk_pixbuf_get_has_alpha(src),
        gdk_pixbuf_get_bits_per_sample(src),
        width,
        height,
        stride,
        NULL,
        NULL);

    return ret;
}

GdkPixbuf * GtkStyledImage__sync_color_with_theme(GtkWidget * parent, GdkPixbuf * src){
    GdkRGBA color;
    GtkStyleContext * context = gtk_widget_get_style_context(parent);// gtk_style_context_new(); 
    gtk_style_context_get_color(context,GTK_STATE_FLAG_NORMAL, &color);
    GdkPixbuf * ret = pixbuf_replace_color(src,&color);
    g_object_unref(src);
    return ret;
}

GdkPixbuf * GtkStyledImage__modify_pixbuf (GtkBinaryImage * self, GdkPixbuf * pixbuf){
    return GtkStyledImage__sync_color_with_theme(GTK_WIDGET(self),pixbuf);
}

static void
GtkStyledImage__class_init (GtkStyledImageClass * klass)
{
    GtkBinaryImageClass *binary_class = GTK_BINARYIMAGE_CLASS(klass);
    binary_class->modify_pixbuf = GtkStyledImage__modify_pixbuf;
}

void GtkStyledImage__style_updated (GtkWidget* self, gpointer user_data)
{
    GtkStyledImagePrivate *priv = GtkStyledImage__get_instance_private (GTK_STYLEDIMAGE(self));
    GdkRGBA color;
    GtkStyleContext * context = gtk_widget_get_style_context(self);// gtk_style_context_new(); 
    gtk_style_context_get_color(context,GTK_STATE_FLAG_NORMAL, &color);

    if(color.red == priv->current_color.red &&
        color.blue == priv->current_color.blue &&
        color.green == priv->current_color.green &&
        color.alpha == priv->current_color.alpha){
        return;
    }

    priv->current_color = color;
    GdkPixbuf * oldpix = gtk_image_get_pixbuf(GTK_IMAGE(self));
    int width = gdk_pixbuf_get_width(oldpix);
    int height = gdk_pixbuf_get_height(oldpix);

    GdkPixbuf * pixbuf = GtkBinaryImage__create_pixbuf(priv->data_start, priv->data_size,NULL);
    if(!pixbuf){
        gtk_image_clear (GTK_IMAGE(self));
        return;
    }
    GdkPixbuf * newpixbuf = GtkStyledImage__sync_color_with_theme(GTK_WIDGET(self),pixbuf);
    GtkBinaryImage__set_image(GTK_BINARYIMAGE(self),newpixbuf, width, height);
    g_object_unref(pixbuf);
}
static void GtkStyledImage__map (GtkStyledImage * self)
{
    g_signal_connect (self, "style-updated", G_CALLBACK (GtkStyledImage__style_updated), NULL);
}

static void GtkStyledImage__realize (GtkStyledImage * self, struct GSIData * data)
{
    GtkStyledImagePrivate *priv = GtkStyledImage__get_instance_private (GTK_STYLEDIMAGE(self));
    GtkStyleContext * context = gtk_widget_get_style_context(GTK_WIDGET(self));// gtk_style_context_new(); 
    gtk_style_context_get_color(context,GTK_STATE_FLAG_NORMAL, &priv->current_color);

    GtkBinaryImage__set_image(GTK_BINARYIMAGE(self),data->pixbuf, data->width, data->height);
    g_object_unref(data->pixbuf);
    free(data);
}

static void
GtkStyledImage__init (GtkStyledImage * self)
{
    GtkStyledImagePrivate *priv = GtkStyledImage__get_instance_private (self);
    priv->data_start = NULL;
    priv->data_size = 0;

    g_signal_connect_after (self, "map", G_CALLBACK (GtkStyledImage__map), NULL);
}

GtkWidget* GtkStyledImage__new(unsigned char * data_start, unsigned int data_size, int width, int height, GError *error){
    GdkPixbuf * pixbuf = GtkBinaryImage__create_pixbuf(data_start, data_size,error);
    if(!pixbuf){
        return NULL;
    }

    GtkImage *image;

    image = g_object_new (GTK_TYPE_STYLEDIMAGE, NULL);
    
    GtkStyledImagePrivate *priv = GtkStyledImage__get_instance_private (GTK_STYLEDIMAGE(image));
    priv->data_start = data_start;
    priv->data_size = data_size;

    struct GSIData * data = malloc(sizeof(struct GSIData));
    data->pixbuf = pixbuf;
    data->height = height;
    data->width = width;

    //Setting the image after realize in order to properly pickup theme color from widget
    g_signal_connect (image, "realize", G_CALLBACK (GtkStyledImage__realize), data);

    return GTK_WIDGET (image);
}