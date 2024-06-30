#include <string.h>
#include "gst/gst_plugin_utils.h"
#include "portable_thread.h"
#include "app/onvif_app.h"
#include <gst/pbutils/gstpluginsbaseversion.h>
#include <gtk/gtk.h>
#include <execinfo.h>
#include <signal.h>
#include "clogger.h"

static void handler(int sig, siginfo_t *dont_care, void *dont_care_either)
{
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


void handler_signal(int sig) {
  void *array[10];
  size_t size;
  // struct sigaction sigIntHandler;
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char *argv[]) {
  /*
    Debugging handlers
   */
  struct sigaction sa;

  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);

  sa.sa_flags     = SA_NODEFER;
  sa.sa_sigaction = handler;

  sigaction(SIGSEGV, &sa, NULL);
  signal(SIGSEGV, handler_signal);  
  
  c_log_set_level(C_ALL_E);
  c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  
  C_INFO("Onvif Device Manager Version : %d.%d", ONVIFMGR_VERSION_MAJ, ONVIFMGR_VERSION_MIN);
  C_INFO("Using Gstreamer Version : %i.%i.%i.%i",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);

  gst_plugin_init_static();


  C_INFO("**** Video decoders *******");
  gst_print_elements_by_type("video/x-h264");
  gst_print_elements_by_type("video/x-h265");
  gst_print_elements_by_type("image/jpeg");
  gst_print_elements_by_type("video/x-av1");
  C_INFO("****************************");

  C_INFO("**** Audio decoders *******");
  gst_print_elements_by_type("audio/x-mulaw");
  gst_print_elements_by_type("audio/x-alaw");
  gst_print_elements_by_type("audio/mpeg");
  C_INFO("****************************");

  /* Initialize Application */
  OnvifApp__new();

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  gst_deinit ();
  return 0;
}
