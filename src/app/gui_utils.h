#ifndef ONVIF_GUI_UTILS_H_ 
#define ONVIF_GUI_UTILS_H_

#include <gtk/gtk.h>

#define GLIST_FOREACH(item, list) for(GList *__glist = list; __glist && (item = __glist->data, TRUE); __glist = __glist->next)

void gui_signal_emit(gpointer instance, guint singalid, gpointer data);
void gui_widget_destroy(GtkWidget * widget, gpointer user_data);
void gui_container_remove(GtkWidget * widget, gpointer user_data);
void gui_update_widget_image(GtkWidget * image, GtkWidget * handle);
void gui_set_label_text (GtkWidget * widget, char * value);
GtkWidget * add_label_entry(GtkWidget * grid, int row, char* lbl);
void safely_destroy_widget(GtkWidget * widget);
void safely_start_spinner(GtkWidget * widget);
void gui_widget_make_square(GtkWidget * widget);
void gui_widget_set_css(GtkWidget * widget, char * css);
void safely_set_widget_css(GtkWidget * widget, char * css);

#endif