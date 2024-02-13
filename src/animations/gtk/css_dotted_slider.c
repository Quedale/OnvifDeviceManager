#include "css_dotted_slider.h"
#include "custom_gtk_revealer.h"


const char * CSS_DOT_r = "* {\n" \
" border-radius:19px;\n" \
" min-height: 19px;\n" \
" min-width: 19px;\n" \
" background-color: currentColor;\n" \
"}";

static void change_direction (CustomGtkRevealer *revealer){
    gboolean revealed = custom_gtk_revealer_get_reveal_child(CUSTOM_GTK_REVEALER(revealer));
    if (gtk_widget_get_mapped (GTK_WIDGET (revealer))){
        custom_gtk_revealer_set_reveal_child(CUSTOM_GTK_REVEALER(revealer),!revealed);
    } else if(revealed) {
        //Finish hiding widgets to avoid breaking state between hiding/showing the slider
        custom_gtk_revealer_set_reveal_child(CUSTOM_GTK_REVEALER(revealer),!revealed);
    }
}

void dotted_map_event (GtkWidget* self, gpointer user_data){
    custom_gtk_revealer_restart(CUSTOM_GTK_REVEALER(self));
    custom_gtk_revealer_set_reveal_child(CUSTOM_GTK_REVEALER(self),TRUE);
}

GtkWidget * css_dotted_slider_animation_new(int item_count,unsigned int animation_time){
    GtkCssProvider * cssProvider;
    GtkStyleContext * context;
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    double delay;

    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider,CSS_DOT_r,-1,NULL); 
    
    for(int i=0;i<item_count;i++){
        delay = (double) animation_time / item_count * i - animation_time / item_count;

        GtkWidget * revealer = custom_gtk_revealer_new();
        custom_gtk_revealer_set_transition_type(CUSTOM_GTK_REVEALER(revealer),CUSTOM_GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
        custom_gtk_revealer_set_transition_duration(CUSTOM_GTK_REVEALER(revealer),0);
        custom_gtk_revealer_set_hide_transition_duration(CUSTOM_GTK_REVEALER(revealer),animation_time*1000);
        custom_gtk_revealer_set_start_delay(CUSTOM_GTK_REVEALER(revealer),delay*1000);

        g_signal_connect (revealer, "notify::child-revealed", G_CALLBACK (change_direction), NULL);
        g_signal_connect (revealer, "map", G_CALLBACK (dotted_map_event), NULL);

        GtkWidget * widget = gtk_label_new("");

        context = gtk_widget_get_style_context(widget);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
        gtk_container_add(GTK_CONTAINER(revealer),widget);
    
        gtk_box_pack_start(GTK_BOX(vbox), revealer, TRUE, FALSE, 0);

    }

    g_object_unref (cssProvider); 

    return vbox;
}