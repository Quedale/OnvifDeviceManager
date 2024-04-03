#ifndef ONVIF_STATIC_PLUGINS_H_ 
#define ONVIF_STATIC_PLUGINS_H_

void
gst_plugin_init_static (void);

void
gst_plugin_feature_set_rank_by_name(char * element_name, int priority);

int 
gst_version_compare(int major, int minor, int micro);

void 
gst_print_elements_by_type(char * type);

#endif