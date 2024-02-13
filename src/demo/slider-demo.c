
#include "../src/animations/gtk/css_dotted_slider.h"


/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
}


int main(int argc, char *argv[])
{

    /* Initialize GTK */
    gtk_init (&argc, &argv);

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "GtkDottedSlider demo");

    GtkWidget * w = css_dotted_slider_animation_new(10,1);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), w,     TRUE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window),vbox);


    gtk_window_set_default_size(GTK_WINDOW(window),100,100);
    gtk_widget_show_all (window);

    /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
    gtk_main ();

}