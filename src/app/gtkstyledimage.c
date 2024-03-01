#include "gtkstyledimage.h"
#include "gui_utils.h"
#include "clogger.h"


typedef struct {
    GdkRGBA current_color;
} GtkStyledImagePrivate;

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

    unsigned char* data = GtkBinaryImage__get_data(GTK_BINARYIMAGE(self));
    unsigned int size = GtkBinaryImage__get_size(GTK_BINARYIMAGE(self));
    GdkPixbuf * pixbuf = GtkBinaryImage__create_pixbuf(data, size,NULL);
    if(!pixbuf){
        gtk_image_clear (GTK_IMAGE(self));
        return;
    }
    GdkPixbuf * newpixbuf = GtkStyledImage__sync_color_with_theme(GTK_WIDGET(self),pixbuf);
    GtkBinaryImage__set_image(GTK_BINARYIMAGE(self),newpixbuf);
    g_object_unref(pixbuf);
}


static void
GtkStyledImage__init (GtkStyledImage * self)
{
    g_signal_connect (self, "style-updated", G_CALLBACK (GtkStyledImage__style_updated), NULL);
}

GtkWidget* GtkStyledImage__new(unsigned char * data_start, unsigned int data_size, int width, int height, GError *error){
    GtkImage *image;

    image = g_object_new (GTK_TYPE_STYLEDIMAGE, NULL);
    GtkBinaryImage__set_height(GTK_BINARYIMAGE(image),height);
    GtkBinaryImage__set_width(GTK_BINARYIMAGE(image),width);
    GtkBinaryImage__set_data(GTK_BINARYIMAGE(image),data_start,data_size,error);
    if(!GtkBinaryImage__has_data(GTK_BINARYIMAGE(image))){
        g_object_unref(image);
        return NULL;
    }

    return GTK_WIDGET (image);
}