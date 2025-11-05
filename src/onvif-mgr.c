#include <string.h>
#include "gst/gst_plugin_utils.h"
#include "portable_thread.h"
#include "app/onvif_app.h"
#include <gst/pbutils/gstpluginsbaseversion.h>
#include <gtk/gtk.h>
#include <execinfo.h>
#include <signal.h>
#include "clogger.h"
#include <argp.h>
#include <gdk/gdk.h>

static struct argp_option options[] = {
    { "loglevel",       'l',    "INT",     0,  "Set the application log level. (Default: 6)", 1},
    { 0 }
};

//Define argument struct
struct arguments {
    int log_level;
};

//main arguments processing function
static error_t 
parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'l': arguments->log_level = atoi(arg); break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

static char doc[] = "OnvifManager";

//Initialize argp context
static struct argp argp = { options, parse_opt, NULL, doc, 0, 0, 0 };


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
  fprintf(stderr, "[%ld] Error: signal %d:\n", P_THREAD_ID, sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


static void 
set_element_priority(char * element_name, int priority){
  GstRegistry* plugins_register = gst_registry_get();
  GstPluginFeature* plugfeat = gst_registry_lookup_feature(plugins_register, element_name);

  if(plugfeat == NULL) {
      C_WARN("Element '%s' not found.",element_name);
      return;
  }

  int rank = gst_plugin_feature_get_rank(plugfeat);
  if(rank > priority){
    C_WARN("Found element '%s'. Lowering priority from %d to %d...",element_name,rank,priority);
    gst_plugin_feature_set_rank(plugfeat, priority);
  } else if(rank < priority){
    C_WARN("Found element '%s'. Increasing priority from %d to %d...",element_name,rank,priority);
    gst_plugin_feature_set_rank(plugfeat, priority);
  }

  gst_object_unref(plugfeat);
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
  c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);

  struct arguments arguments;

  /* Default values. */
  arguments.log_level = C_TRACE_E;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if(arguments.log_level < -1){
    arguments.log_level = C_OFF_E;
    C_WARN("Invalid loglevel parameter. Should be between -1 and %d",C_ALL_E);
  } else if(arguments.log_level > C_ALL_E){
    arguments.log_level = C_ALL_E;
    C_WARN("Invalid loglevel parameter. Should be between -1 and %d",C_ALL_E);
  }
  c_log_set_level(arguments.log_level);
  /* Initialize GTK */
  gtk_init (&argc, &argv);
  gtk_window_set_default_icon_name("io.github.quedale.onvifmgr");
  gdk_set_program_class("io.github.quedale.onvifmgr");
  g_set_prgname("io.github.quedale.onvifmgr");

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  
  //Dropping vaapi plugin priority under libav since I can't get it to work properly under flatpak. TODO: Fix this?
  set_element_priority("vah264dec",GST_RANK_PRIMARY-1);
  set_element_priority("vah265dec",GST_RANK_PRIMARY-1);
  set_element_priority("vaav1dec",GST_RANK_PRIMARY-1);

  C_INFO("Onvif Manager Version : %d.%d", ONVIFMGR_VERSION_MAJ, ONVIFMGR_VERSION_MIN);
  C_INFO("Gstreamer Version : %i.%i.%i.%i",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);
  C_INFO("GTK Version : %d.%d.%d", gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version());
  C_INFO("GLib Version %d.%d.%d", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
  C_INFO("GLibc Version %d.%d", __GLIBC__, __GLIBC_MINOR__);
  #ifdef GDK_WINDOWING_WAYLAND
      C_INFO("Display Type : Wayland"); 
  #elif GDK_WINDOWING_X11
      C_INFO("Display Type : X11"); 
  #else
      C_INFO("Display Type : Unkown"); 
  #endif

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
