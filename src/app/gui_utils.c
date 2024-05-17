
#include "gui_utils.h"
#include "clogger.h"

typedef struct {
    GtkWidget * image;
    GtkWidget * handle;
} ImageGUIUpdate;

typedef struct {
    GtkWidget * label;
    char * text;
} GUILabelUpdate;

typedef struct {
    gpointer instance;
    guint signalid;
    gpointer param;
} IdleSignalEmit;

void idle_signal_emit (void * user_data){
    IdleSignalEmit * data = (IdleSignalEmit*)user_data;
    g_signal_emit (data->instance, data->signalid, 0, data->param);
    free(data);
}

void gui_signal_emit(gpointer instance, guint singalid, gpointer param){
    IdleSignalEmit * emit = malloc(sizeof(IdleSignalEmit));
    emit->instance= instance;
    emit->signalid = singalid;
    emit->param = param;
    gdk_threads_add_idle(G_SOURCE_FUNC(idle_signal_emit),emit);
}

void gui_widget_destroy(GtkWidget * widget, gpointer user_data){
    gtk_widget_destroy(widget);
}

void gui_container_remove(GtkWidget * widget, gpointer user_data){
    gtk_container_remove(GTK_CONTAINER(user_data),widget);
}

gboolean * gui_update_widget_image_priv(void * user_data){
    ImageGUIUpdate * iguiu = (ImageGUIUpdate *) user_data;

    if(GTK_IS_WIDGET(iguiu->handle)){
        gtk_container_foreach (GTK_CONTAINER (iguiu->handle), (GtkCallback)gui_widget_destroy, NULL);
        if(GTK_IS_WIDGET(iguiu->image)){
            gtk_container_add (GTK_CONTAINER (iguiu->handle), iguiu->image);
            gtk_widget_show (iguiu->image);
            if(GTK_IS_SPINNER(iguiu->image)){
                gtk_spinner_start (GTK_SPINNER (iguiu->image));
            }
        }
    } else {
        C_WARN("gui_update_widget_image_priv - invalid handle");
    }

    free(iguiu);
    return FALSE;
}

void gui_update_widget_image(GtkWidget * image, GtkWidget * handle){
    if(!G_IS_OBJECT(image) || !G_IS_OBJECT(handle)) return;
    ImageGUIUpdate * iguiu = malloc(sizeof(ImageGUIUpdate));
    iguiu->image = image;
    iguiu->handle = handle;
    gdk_threads_add_idle(G_SOURCE_FUNC(gui_update_widget_image_priv),iguiu);
}

gboolean * gui_set_label_text_priv (void * user_data){
    GUILabelUpdate * update = (GUILabelUpdate *) user_data;
    gtk_label_set_text(GTK_LABEL(update->label),update->text);

    free(update->text);
    free(update);

    return FALSE;
}

void gui_set_label_text (GtkWidget * widget, char * value){
    if(!G_IS_OBJECT(widget) || !G_IS_OBJECT(widget)) return;
    GUILabelUpdate * iguiu = malloc(sizeof(GUILabelUpdate));
    iguiu->text = malloc(strlen(value)+1);
    strcpy(iguiu->text,value);
    iguiu->label = widget;
    gdk_threads_add_idle(G_SOURCE_FUNC(gui_set_label_text_priv),iguiu);
}

gboolean * gui_widget_destroy_cb (void * user_data){
    GtkWidget * widget = (GtkWidget *) user_data;
    if(GTK_IS_WIDGET(widget)){
        gtk_widget_destroy(widget);
    }
    return FALSE;
}

void safely_destroy_widget(GtkWidget * widget){
    gdk_threads_add_idle(G_SOURCE_FUNC(gui_widget_destroy_cb),widget);
}

void idle_start_spinner(void * user_data){
    GtkWidget * widget = (GtkWidget *) user_data;
    if(GTK_IS_SPINNER(widget)){
        gtk_spinner_start (GTK_SPINNER (widget));
    }
}

void safely_start_spinner(GtkWidget * widget){
    gdk_threads_add_idle(G_SOURCE_FUNC(idle_start_spinner),widget);
}

GtkCssProvider * gui_widget_set_css(GtkWidget * widget, char * css, GtkCssProvider * cssProvider){
    if(!cssProvider){
        cssProvider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(cssProvider, css,-1,NULL);  //font-size: 25px; 
    }

    GtkStyleContext * context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    return cssProvider;
}

gboolean gui_realize_make_square (GtkWidget *widget, gpointer p, gpointer player){
    GtkRequisition max_size;
    gtk_widget_get_preferred_size(widget,&max_size,NULL);
    if(max_size.height > max_size.width){
        gtk_widget_set_size_request(widget,max_size.height,max_size.height);
    } else if(max_size.height < max_size.width){
        gtk_widget_set_size_request(widget,max_size.width,max_size.width);
    }
    return FALSE;
}

void gui_widget_make_square(GtkWidget * widget){
    g_signal_connect (widget, "realize", G_CALLBACK (gui_realize_make_square), NULL);
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