#include <string.h>
#include "gst/onvifinitstaticplugins.h"
#include "app/onvif_app.h"
#include <gst/pbutils/gstpluginsbaseversion.h>
#include <gtk/gtk.h>
#include <execinfo.h>
#include <signal.h>

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char *argv[]) {
  // signal(SIGSEGV, handler);   // install our handler

  // make OpenSSL MT-safe with mutex
  // CRYPTO_thread_setup();
  

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  onvif_init_static_plugins();

  gst_debug_set_threshold_for_name ("ext-gst-player", GST_LEVEL_DEBUG);
  
  printf("Using Gstreamer Version : %i.%i.%i.%i\n",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);
  
  /* Initialize Application */
  OnvifApp__create();

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  return 0;
}
