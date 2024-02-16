#include "gtkstyledimage.h"
#include "gui_utils.h"
#include "clogger.h"

G_DEFINE_TYPE(GtkBinaryImage, GtkBinaryImage_, GTK_TYPE_IMAGE)

static void
GtkBinaryImage__class_init (GtkBinaryImageClass * klass)
{
    klass->modify_pixbuf = NULL;
}

static void
GtkBinaryImage__init (GtkBinaryImage * self)
{

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

    pixbuf = gdk_pixbuf_copy(pixbuf); //TODO Create virtual method to override with styledimage

    gdk_pixbuf_loader_close(loader,NULL);
    g_object_unref(loader);

    g_object_ref(pixbuf);
    return pixbuf;
}

void GtkBinaryImage__set_image(GtkBinaryImage *  self, GdkPixbuf * pixbuf, int width, int height){
    if(!pixbuf){
        C_WARN("No GdkPixbuf set on image. Not scaling...");
        return;
    }

    double ph = gdk_pixbuf_get_height (pixbuf);
    double pw = gdk_pixbuf_get_width (pixbuf);
    if(width == -1){
        pw = height / ph * pw;
        ph = height;
    } else if(height == -1){
        ph = width / ph * pw;
        pw = width;
    } else {
        ph = height;
        pw = width;
    }

    if(GTK_BINARYIMAGE_GET_CLASS (self)->modify_pixbuf){
        pixbuf = GTK_BINARYIMAGE_GET_CLASS (self)->modify_pixbuf (self, pixbuf);
    }
    
    GdkPixbuf * newbuf = gdk_pixbuf_scale_simple (pixbuf,pw,ph,GDK_INTERP_NEAREST);
    g_object_unref(pixbuf);
    gtk_image_set_from_pixbuf(GTK_IMAGE(self),newbuf);
    g_object_unref(newbuf);
}

GtkWidget* GtkBinaryImage__new(unsigned char * data_start, unsigned int data_size, int width, int height, GError *error){
    GdkPixbuf * pixbuf = GtkBinaryImage__create_pixbuf(data_start, data_size,error);
    if(!pixbuf){
        return NULL;
    }

    GtkImage *image;

    image = g_object_new (GTK_TYPE_BINARYIMAGE, NULL);

    GtkBinaryImage__set_image(GTK_BINARYIMAGE(image), pixbuf, width, height);
    g_object_unref()
    return GTK_WIDGET (image);
}