
#include "gui_utils.h"

typedef struct {
    GtkWidget * image;
    GtkWidget * handle;
} ImageGUIUpdate;

gboolean * gui_update_widget_image_priv(void * user_data){
    ImageGUIUpdate * iguiu = (ImageGUIUpdate *) user_data;
    printf("gui_update_widget_image_priv\n");

    if(GTK_IS_WIDGET(iguiu->handle)){
        gtk_container_foreach (GTK_CONTAINER (iguiu->handle), (void*) gtk_widget_destroy, NULL);
        if(iguiu->image){
            gtk_container_add (GTK_CONTAINER (iguiu->handle), iguiu->image);
            gtk_widget_show (iguiu->image);
        }
    } else {
        printf("gui_update_widget_image_priv - invalid handle\n");
    }

    free(iguiu);

    return FALSE;
}

void gui_update_widget_image(GtkWidget * image, GtkWidget * handle){
    ImageGUIUpdate * iguiu = malloc(sizeof(ImageGUIUpdate));
    iguiu->image = image;
    iguiu->handle = handle;
    gdk_threads_add_idle((void *)gui_update_widget_image_priv,iguiu);
}


gboolean * gui_widget_destroy (void * user_data){
    GtkWidget * widget = (GtkWidget *) user_data;
    gtk_widget_destroy(widget);
    return FALSE;
}

void safely_destroy_widget(GtkWidget * widget){
    gdk_threads_add_idle((void *)gui_widget_destroy,widget);
}

GtkWidget * add_label_entry(GtkWidget * grid, int row, char* lbl){
    GtkWidget * widget;

    widget = gtk_label_new (lbl);
    gtk_widget_set_size_request(widget,130,-1);
    gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, row, 1, 1);
    widget = gtk_entry_new();
    gtk_widget_set_size_request(widget,300,-1);
    gtk_entry_set_text(GTK_ENTRY(widget),"");
    gtk_editable_set_editable  ((GtkEditable*)widget, FALSE);
    gtk_grid_attach (GTK_GRID (grid), widget, 1, row, 1, 1);

    return widget;
}