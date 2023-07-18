#include "dotted_slider.h"
#include <stdlib.h>

#define DOTTED_SLIDER_ID_PREFIX "dsa_%d"
#define DOTTED_SLIDER_CSS_DATA " #dsa_%d { animation-delay:%.2fs; }"

GtkWidget * create_dotted_slider_animation(){
  GtkBuilder *builder;
  GtkWidget * slider;

  builder = gtk_builder_new_from_file("./src/animations/glade.glade");
  if(!builder){
    exit(1);
  }

  slider = gtk_builder_get_object(builder, "dsa_");

  if(!slider){
    exit(1);
  }

  // Build signal structure
  gtk_builder_connect_signals(builder, NULL);

  // Prepare css file
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(provider, "./src/animations/css.css", NULL);

  gtk_style_context_add_provider_for_screen(
    gdk_display_get_default_screen(gdk_display_get_default()),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  return GTK_WIDGET(slider);
}

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

  gtk_window_set_title (GTK_WINDOW (main_window), "Dotted Slider Demo");

  GtkWidget * slider = create_dotted_slider_animation();

  gtk_container_add (GTK_CONTAINER (main_window), slider);

  gtk_widget_show_all (main_window);

  gtk_main();

  return (EXIT_SUCCESS);
}