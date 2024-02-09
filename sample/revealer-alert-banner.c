#include <gtk/gtk.h>
#include "../src/animations/gtk/custom_gtk_revealer.h"

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
}

static void
change_direction (CustomGtkRevealer *revealer)
{
  if (gtk_widget_get_mapped (GTK_WIDGET (revealer)))
    {
      gboolean revealed;

      revealed = custom_gtk_revealer_get_child_revealed (revealer);
      custom_gtk_revealer_set_reveal_child (revealer, !revealed);
    }
}

int main(int argc, char *argv[])
{
    /* Initialize GTK */
    gtk_init (&argc, &argv);

    GtkWidget *main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    gtk_window_set_title (GTK_WINDOW (main_window), "CustomGtkRevealer");
    gtk_window_set_default_size(GTK_WINDOW(main_window),100,100);

    GtkWidget * revealer = custom_gtk_revealer_new();
    custom_gtk_revealer_set_transition_duration(CUSTOM_GTK_REVEALER(revealer),500);
    custom_gtk_revealer_set_transition_type(CUSTOM_GTK_REVEALER(revealer),CUSTOM_GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
    g_signal_connect (revealer, "notify::child-revealed",
                        G_CALLBACK (change_direction), NULL);

    GtkWidget * label = gtk_label_new("Some alert label styled with CSS");

    gtk_container_add (GTK_CONTAINER (revealer), label);
    gtk_container_add (GTK_CONTAINER (main_window), revealer);
    gtk_widget_show_all (main_window);
    
    custom_gtk_revealer_set_delay(CUSTOM_GTK_REVEALER(revealer),2000);
    // custom_gtk_revealer_set_start_delay(CUSTOM_GTK_REVEALER(revealer),4000);
    // custom_gtk_revealer_set_hide_delay(CUSTOM_GTK_REVEALER(revealer),2000);
    // custom_gtk_revealer_set_show_delay(CUSTOM_GTK_REVEALER(revealer),2000);

    custom_gtk_revealer_set_reveal_child(CUSTOM_GTK_REVEALER(revealer),1);

    gtk_main ();

}