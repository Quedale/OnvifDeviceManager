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
/*
  Due to a segmentation fault error cause by libva avdec_h264, dropping its priority
  Experienced using system shared library version 1.20.1
  Apparently this version of libva doesn't like to get the pipleline destroyed and recreated
*/
void drop_libvah264_priority(){
  GList * factories;
  GList *tmp;
  GstCaps * caps = gst_caps_new_empty_simple("video/x-h264");
  factories = gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_DECODER || GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, GST_RANK_MARGINAL);
  factories = gst_element_factory_list_filter (factories, caps, GST_PAD_SINK, FALSE);
  for (tmp = factories; tmp; tmp = tmp->next) {
      GstElementFactory *fact = (GstElementFactory *) tmp->data;
      if (gst_element_factory_list_is_type (fact,GST_ELEMENT_FACTORY_TYPE_DECODER || GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO)){
          int rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE_CAST(fact));
          char * name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE_CAST(fact));
          C_DEBUG("Gstreamer encoder element : %s[%d]",name,rank);
          if(!version_compare(1,22,8) &&  //Im not sure which version is causing it, but I know it works well on 1.22.8 crashes on 1.20.1
            strcmp(name,"avdec_h264") == 0){
              C_WARN("Found element 'avdec_h264'. Lowering priority to 32...");
              gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE_CAST(fact),32);
          }
      } 
      else {
          int rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE_CAST(fact));
          char * name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE_CAST(fact));
          printf("NOT decoder type : %s [%d] \n",name, rank);
      }
  }
  g_list_free (factories);
}

int main(int argc, char *argv[]) {
  // signal(SIGSEGV, handler);   // install our handler

  // make OpenSSL MT-safe with mutex
  // CRYPTO_thread_setup();
  
  c_log_set_level(C_ALL_E);

  /* Initialize GTK */
  gtk_init (&argc, &argv);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);
  onvif_init_static_plugins();
  drop_libvah264_priority();


  gst_debug_set_threshold_for_name ("ext-gst-player", GST_LEVEL_DEBUG);
  
  C_INFO("Using Gstreamer Version : %i.%i.%i.%i",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);
  
  /* Initialize Application */
  OnvifApp__create();

  /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
  gtk_main ();

  gst_deinit ();
  return 0;
}
