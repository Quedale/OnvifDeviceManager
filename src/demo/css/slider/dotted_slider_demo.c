#include "../../../animations/dotted_slider.h"


/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, void *data) {
  gtk_main_quit ();
}

int
main (int argc, char **argv)
{   

  gtk_init(&argc, &argv);
  GtkWidget * main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), NULL);

  gtk_window_set_title (GTK_WINDOW (main_window), "Slider demo");

  double item_count;
  double animation_time;

  item_count = 40;
  animation_time = 1;

  GtkWidget * spinner = create_dotted_slider_animation(item_count,animation_time);

  gtk_container_add (GTK_CONTAINER (main_window), spinner);

  gtk_widget_show_all (main_window);

  gtk_main();

  return (EXIT_SUCCESS);
}