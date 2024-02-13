#ifndef ONVIF_GUI_UTILS_H_ 
#define ONVIF_GUI_UTILS_H_

#include <gtk/gtk.h>

#define GLIST_FOREACH(item, list) for(GList *__glist = list; __glist && (item = __glist->data, TRUE); __glist = __glist->next)

void gui_widget_destroy(GtkWidget * widget, gpointer user_data);
void gui_container_remove(GtkWidget * widget, gpointer user_data);
void gui_update_widget_image(GtkWidget * image, GtkWidget * handle);
GtkWidget * add_label_entry(GtkWidget * grid, int row, char* lbl);
void safely_destroy_widget(GtkWidget * widget);

#endif