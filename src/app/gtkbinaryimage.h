#ifndef GTK_BINARYIMAGE_H_ 
#define GTK_BINARYIMAGE_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GTK_TYPE_BINARYIMAGE GtkBinaryImage__get_type()
G_DECLARE_DERIVABLE_TYPE (GtkBinaryImage, GtkBinaryImage_, GTK, BINARYIMAGE, GtkImage)

struct _GtkBinaryImageClass
{
  GtkImageClass parent_class;
  
  GdkPixbuf * (* modify_pixbuf) (GtkBinaryImage * self, GdkPixbuf * pixbuf);
};

GtkWidget* GtkBinaryImage__new(unsigned char * data_start, unsigned int data_size, int width, int height, GError * error);
GdkPixbuf * GtkBinaryImage__create_pixbuf(unsigned char* data_start, unsigned int data_size, GError * error);
void GtkBinaryImage__set_image(GtkBinaryImage *  img, GdkPixbuf * pixbuf);
void GtkBinaryImage__set_width(GtkBinaryImage *  self, int width);
void GtkBinaryImage__set_height(GtkBinaryImage *  self, int height);
void GtkBinaryImage__set_data(GtkBinaryImage *  self, unsigned char* data_start, unsigned int data_siz, GError *errore);
int GtkBinaryImage__has_data(GtkBinaryImage *  self);
unsigned char* GtkBinaryImage__get_data(GtkBinaryImage *  self);
unsigned int GtkBinaryImage__get_size(GtkBinaryImage *  self);

G_END_DECLS

#endif