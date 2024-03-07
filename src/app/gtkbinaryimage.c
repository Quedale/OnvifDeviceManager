#include "gtkstyledimage.h"
#include "gui_utils.h"
#include "clogger.h"

typedef struct {
    unsigned char * data_start;
    unsigned int data_size;
    int height;
    int width;
    double dpi_factor;
} GtkBinaryImagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GtkBinaryImage, GtkBinaryImage_, GTK_TYPE_IMAGE)

static void
GtkBinaryImage__class_init (GtkBinaryImageClass * klass)
{
    klass->modify_pixbuf = NULL;
}

static void
GtkBinaryImage__screen_changed (GtkWidget* self, GdkScreen* previous_screen, gpointer user_data){
    GdkScreen * scrn = gtk_widget_get_screen(GTK_WIDGET(self));
    double resolution = gdk_screen_get_resolution(scrn)/96;

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    if(priv->dpi_factor != resolution){
        //TODO reload image with new dpi factor
        C_FATAL("SCREE DPI CHANGED %f/%f",resolution,priv->dpi_factor);
    }
}

static void
GtkBinaryImage__init (GtkBinaryImage * self)
{
    g_signal_connect_after (self, "screen-changed", G_CALLBACK (GtkBinaryImage__screen_changed), NULL);

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));

    GdkScreen * scrn = gtk_widget_get_screen(GTK_WIDGET(self));
    priv->dpi_factor = gdk_screen_get_resolution(scrn)/96;
    priv->height = -1;
    priv->width = -1;
}

GdkPixbuf * GtkBinaryImage__create_pixbuf(unsigned char* data_start, unsigned int data_size, GError * error){
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
    if(!gdk_pixbuf_loader_write (loader, (unsigned char *)data_start, data_size, &error)){
        return NULL;
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if(!pixbuf){
        return NULL;
    }

    g_object_ref(pixbuf);

    gdk_pixbuf_loader_close(loader,NULL);
    g_object_unref(loader);

    g_object_ref(pixbuf);
    return pixbuf;
}

void GtkBinaryImage__set_image(GtkBinaryImage *  self, GdkPixbuf * pixbuf){
    g_return_if_fail (GTK_IS_BINARYIMAGE (self));

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    if(!pixbuf){
        C_WARN("No GdkPixbuf set on image. Not scaling...");
        return;
    }

    double ph = gdk_pixbuf_get_height (pixbuf);
    double pw = gdk_pixbuf_get_width (pixbuf);

    if(GTK_BINARYIMAGE_GET_CLASS (self)->modify_pixbuf){
        pixbuf = GTK_BINARYIMAGE_GET_CLASS (self)->modify_pixbuf (self, pixbuf);
    }

    GdkPixbuf * newbuf;
    if(priv->width != -1 || priv->height != -1){
        if(priv->width == -1){
            pw = priv->height / ph * pw;
            ph = priv->height;
        } else if(priv->height == -1){
            ph = priv->width / ph * pw;
            pw = priv->width;
        } else {
            ph = priv->height;
            pw = priv->width;
        }

        GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
        newbuf = gdk_pixbuf_scale_simple (pixbuf,pw*priv->dpi_factor,ph*priv->dpi_factor,GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
    } else {
        newbuf = pixbuf;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(self),newbuf);
    g_object_unref(newbuf);
}

GtkWidget* GtkBinaryImage__new(unsigned char * data_start, unsigned int data_size, int width, int height, GError *error){

    GtkImage *image;

    image = g_object_new (GTK_TYPE_BINARYIMAGE, NULL);
    GtkBinaryImage__set_height(GTK_BINARYIMAGE(image),height);
    GtkBinaryImage__set_width(GTK_BINARYIMAGE(image),width);
    GtkBinaryImage__set_data(GTK_BINARYIMAGE(image),data_start,data_size,error);
    if(!GtkBinaryImage__has_data(GTK_BINARYIMAGE(image))){
        return NULL;
    }
    return GTK_WIDGET (image);
}

void GtkBinaryImage__set_data(GtkBinaryImage *  self, unsigned char* data_start, unsigned int data_size, GError *error){
    g_return_if_fail (GTK_IS_BINARYIMAGE (self));

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));

    GdkPixbuf * pixbuf = GtkBinaryImage__create_pixbuf(data_start, data_size,error);
    if(!pixbuf){
        return;
    }

    priv->data_start = data_start;
    priv->data_size = data_size;

    GtkBinaryImage__set_image(GTK_BINARYIMAGE(self), pixbuf);
}

void GtkBinaryImage__set_width(GtkBinaryImage *  self, int width){
    g_return_if_fail (GTK_IS_BINARYIMAGE (self));

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    priv->width = width;

}
void GtkBinaryImage__set_height(GtkBinaryImage *  self, int height){
    g_return_if_fail (GTK_IS_BINARYIMAGE (self));

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    priv->height = height;
}

int GtkBinaryImage__has_data(GtkBinaryImage *  self){
    g_return_val_if_fail (GTK_IS_BINARYIMAGE (self),0);

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    return priv->data_start != NULL;
}

unsigned char* GtkBinaryImage__get_data(GtkBinaryImage *  self){
    g_return_val_if_fail (GTK_IS_BINARYIMAGE (self),NULL);

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    return priv->data_start;
}

unsigned int GtkBinaryImage__get_size(GtkBinaryImage *  self){
    g_return_val_if_fail (GTK_IS_BINARYIMAGE (self),0);

    GtkBinaryImagePrivate *priv = GtkBinaryImage__get_instance_private (GTK_BINARYIMAGE(self));
    return priv->data_size;
}