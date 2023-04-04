#include <string.h>
#include "gst2/onvifinitstaticplugins.h"
#include "app/onvif_app.h"
#include <gst/pbutils/gstpluginsbaseversion.h>
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  onvif_init_static_plugins();

  printf("Using Gstreamer Version : %i.%i.%i.%i\n",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);
  
  /* Initialize Application */
  OnvifApp__create();

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  return 0;
}
