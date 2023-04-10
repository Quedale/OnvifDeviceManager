#ifndef ONVIF_GUI_UTILS_H_ 
#define ONVIF_GUI_UTILS_H_

#include <gtk/gtk.h>

void gui_update_widget_image(GtkWidget * image, GtkWidget * handle);
GtkWidget * add_label_entry(GtkWidget * grid, int row, char* lbl);

#endif