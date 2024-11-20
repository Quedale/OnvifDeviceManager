#include <gtk/gtk.h>
#include "clogger.h"
#include "portable_thread.h"
#include "../app/gtkstyledimage.h"
#include "../app/gui_utils.h"

int color_index = 0;
int color_length = 4;
const char * colors[] = { "currentColor", "red", "green", "blue" };
extern char _binary_microphone_png_size[];
extern char _binary_microphone_png_start[];
extern char _binary_microphone_png_end[];

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_widget_destroy(widget);
    gtk_main_quit();
}


static void change_color (GtkWidget * widget, gpointer * ptr){
    color_index++;
    if(color_index >= color_length){
        color_index = 0;
    }

    char str[255];
    sprintf(str, "* { color: %s; }", colors[color_index]);
    gui_widget_set_css(widget, str);
}

GtkWidget * create_image(){
    GtkWidget * widget = gtk_button_new ();

    GdkRGBA color;
    GtkStyleContext * context = gtk_style_context_new();
    gtk_style_context_get_color(context,GTK_STATE_FLAG_NORMAL, &color);


    GError * error = NULL;
    GtkWidget * image = GtkStyledImage__new((unsigned char *)_binary_microphone_png_start, _binary_microphone_png_end - _binary_microphone_png_start, 30, 30, error);


    gtk_button_set_image (GTK_BUTTON (widget), image);

    g_signal_connect (widget, "clicked", G_CALLBACK (change_color), NULL);

    return widget;
}

int main(int argc, char *argv[])
{
    /* Initialize GTK */
    gtk_init (&argc, &argv);
    c_log_set_level(C_ALL_E);
    c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);
    
    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "GtkDottedSlider Widget demo");

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget * label = gtk_label_new("Click the button to change color dynamically\nWhen the CSS color is set to 'currentColor', it will follow the system theme.");
    gtk_box_pack_start(GTK_BOX(vbox), label,     TRUE, TRUE, 0);

    GtkWidget * image = create_image();
    gtk_box_pack_start(GTK_BOX(vbox), image,     TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(window),vbox);
    
    gtk_window_set_default_size(GTK_WINDOW(window),100,100);
    gtk_widget_show_all (window);

    gtk_main ();

}