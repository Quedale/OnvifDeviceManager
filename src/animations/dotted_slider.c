#include "dotted_slider.h"
#include <stdlib.h>


const char * CSS_DOTTED_SLIDER_r = ".dsa{\n" \
"	animation-name:dot_slid;\n" \
"	animation-duration:%.fs;\n" \
"	animation-iteration-count:infinite;\n" \
"	animation-direction:normal;\n" \
"	border-radius:19px;\n" \
"}\n" \
"@keyframes dot_slid{\n" \
"   0%{\n" \
"       background-color: currentColor;\n" \
"   }\n" \
"   100%{\n" \
"       background-color:rgba(0,0,0,0);\n" \
"   }\n" \
"}\n";

#define DOTTED_SLIDER_ID_PREFIX "dsa_%d"
#define DOTTED_SLIDER_CSS_DATA " #dsa_%d { animation-delay:%.2fs; }"

int GLOBAL_CSS_ADDED = 0;

GtkWidget * create_layout(double item_count, double animation_time){
  GtkStyleContext *context;
  GtkCssProvider *provider;
  GtkWidget * fixed;
  GtkWidget * dot;
  int x = 0;
  double delay;
  char id[100];
  char css_data[100];

  fixed = gtk_fixed_new();

  gtk_widget_set_visible(fixed,1);
  gtk_widget_set_can_focus(fixed,0);
  gtk_widget_set_margin_bottom(fixed,10);
  gtk_widget_set_margin_top(fixed,10);
  gtk_widget_set_margin_start(fixed,10);
  gtk_widget_set_margin_end(fixed,10);  

  for(int i=1;i<=item_count;i++){
    delay = animation_time / item_count * i - animation_time / item_count;
    snprintf(css_data, 100, DOTTED_SLIDER_CSS_DATA, i, delay);    
    snprintf(id, 100, DOTTED_SLIDER_ID_PREFIX,i);

    dot = gtk_label_new("");

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_data, strlen(css_data)+1, NULL);
    context = gtk_widget_get_style_context(dot);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(context,"dsa");

    gtk_widget_set_name(dot,id);
    gtk_widget_set_size_request(dot,20,20);
    gtk_widget_set_visible(dot,1);
    gtk_widget_set_can_focus(dot,0);
    gtk_fixed_put(GTK_FIXED(fixed),dot,x,0);
    x=x+30;

  }

  return fixed;
}

GtkWidget * create_dotted_slider_animation(double item_count, double animation_time){
  GtkWidget * slider;
  GtkCssProvider *provider;

  char css_data[420];
  double factor;

  factor = animation_time / item_count;

  slider = create_layout(item_count, animation_time);

  // Prepare css data
  snprintf(css_data, 420, CSS_DOTTED_SLIDER_r, animation_time, factor,factor);    

  provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider,css_data,-1,NULL);

  if(!GLOBAL_CSS_ADDED){
    gtk_style_context_add_provider_for_screen(
      gdk_display_get_default_screen(gdk_display_get_default()),
      GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      GLOBAL_CSS_ADDED=1;
  }
  
  return GTK_WIDGET(slider);
}