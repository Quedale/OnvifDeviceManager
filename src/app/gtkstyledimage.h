#ifndef GTK_STYLEDIMAGE_H_ 
#define GTK_STYLEDIMAGE_H_

typedef struct _GtkStyledImage GtkStyledImage;

#include <gtk/gtk.h>
#include "gtkbinaryimage.h"

G_BEGIN_DECLS


#define GTK_TYPE_STYLEDIMAGE GtkStyledImage__get_type()
G_DECLARE_FINAL_TYPE (GtkStyledImage, GtkStyledImage_, GTK, STYLEDIMAGE, GtkBinaryImage)


struct _GtkStyledImage
{
  GtkBinaryImage parent_instance;
};


struct _GtkStyledImageClass
{
  GtkBinaryImageClass parent_class;
};

GtkWidget* GtkStyledImage__new(unsigned char * data_start, unsigned int data_size, int width, int height, GError *error);

G_END_DECLS

#endif