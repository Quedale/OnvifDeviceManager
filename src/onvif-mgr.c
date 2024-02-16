#include <string.h>
#include "gst/onvifinitstaticplugins.h"
#include "app/onvif_app.h"
#include <gst/pbutils/gstpluginsbaseversion.h>
#include <gtk/gtk.h>
#include <execinfo.h>
#include <signal.h>
#include "clogger.h"

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

int version_compare(int major, int minor, int micro){
  if(GST_PLUGINS_BASE_VERSION_MAJOR < major){
    return 0;
  } else if(GST_PLUGINS_BASE_VERSION_MAJOR > major){
    return 1;
  }

  if(GST_PLUGINS_BASE_VERSION_MINOR < minor){
    return 0;
  } else if(GST_PLUGINS_BASE_VERSION_MINOR > minor){
    return 1;
  }

  if(GST_PLUGINS_BASE_VERSION_MICRO < micro){
    return 0;
  } else if(GST_PLUGINS_BASE_VERSION_MICRO > micro){
    return 1;
  }

  return 1;
}

void print_elements_by_type(char * type){
  GList * factories, *filtered, *tmp;

  C_INFO("*** %s decoders ***",type);
  GstCaps * caps = gst_caps_new_empty_simple(type);
  factories = gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_DECODER || GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, GST_RANK_MARGINAL);
  filtered = gst_element_factory_list_filter (factories, caps, GST_PAD_SINK, FALSE);
  for (tmp = filtered; tmp; tmp = tmp->next) {
      GstElementFactory *fact = (GstElementFactory *) tmp->data;
      int rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE_CAST(fact));
      char * name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE_CAST(fact));
      C_INFO("*    %s[%d]",name,rank);
  }
  gst_caps_unref (caps);
  g_list_free(filtered);
  g_list_free (factories);
  C_INFO("****************************");
}

/*
  Due to a segmentation fault error cause by libva avdec_h264, dropping its priority
  Experienced using system shared library version 1.20.1
  Apparently this version of libva doesn't like to get the pipleline destroyed and recreated
*/
void set_element_priority(char * element_name, int priority){
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
  signal(SIGSEGV, handler);   // install our handler

  // make OpenSSL MT-safe with mutex
  // CRYPTO_thread_setup();
  
  c_log_set_level(C_ALL_E);
  c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  
  C_INFO("Using Gstreamer Version : %i.%i.%i.%i",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);

  onvif_init_static_plugins();

  //TODO Support setting priority from settings
  set_element_priority("avdec_h264",GST_RANK_MARGINAL); //Can cause a crash
  set_element_priority("avdec_h265",GST_RANK_MARGINAL); //Can cause a crash
  set_element_priority("openh264dec",GST_RANK_MARGINAL+1); //This always works compared to avdec_h264 that can crash

  print_elements_by_type("video/x-h264");
  print_elements_by_type("video/x-h265");
  print_elements_by_type("image/jpeg");
  print_elements_by_type("video/x-av1");
  
  /* Initialize Application */
  OnvifApp__create();

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  gst_deinit ();
  return 0;
}
