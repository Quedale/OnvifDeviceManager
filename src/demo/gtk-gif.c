#include <gtk/gtk.h>
#include <gdk/gdkscreen.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <unistd.h>
#include <stdlib.h>

extern char _binary_loading_gif_size[];
extern char _binary_loading_gif_start[];
extern char _binary_loading_gif_end[];

struct GifTimerInput {
    GdkPixbufAnimationIter *iter;
    GtkWidget * image;
};

static int timer(struct GifTimerInput* input)
{   
    //This cannot be resolved as long as gdk_pixbuf_animation_iter_advance expect a GTimeVal struct
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    GTimeVal val;
    val.tv_sec = 0;
    val.tv_usec = g_get_real_time();
    #pragma GCC diagnostic pop

    //Advance to the right frame based on GTimeVal
    gdk_pixbuf_animation_iter_advance(input->iter, &val);
    //Extract current frame pixbuf
    GdkPixbuf * pixbuff = gdk_pixbuf_animation_iter_get_pixbuf(input->iter);
    //Set pixbuf on GtkImage
    gtk_image_set_from_pixbuf (GTK_IMAGE(input->image),pixbuff);
    //Callback to itself extract next frame
    g_timeout_add(gdk_pixbuf_animation_iter_get_delay_time(input->iter), (GSourceFunc )timer, input);
    return FALSE;
}

void
gif_area_prepared (
  GdkPixbufLoader* self,
  struct GifTimerInput * user_data
){
    //Get PixBufAnimation from loader
    GdkPixbufAnimation * pixbuf_animation = gdk_pixbuf_loader_get_animation(self);
    //Extract initial animation iteration
    user_data->iter = gdk_pixbuf_animation_get_iter (pixbuf_animation,NULL);
    //Initiate image rotation
    timer(user_data);
}

int main(int argc, char **argv)
{
    /* boilerplate initialization code */
    gtk_init(&argc, &argv);

    GtkWidget *window;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Alpha Demo");
    g_signal_connect(G_OBJECT(window), "delete-event", gtk_main_quit, NULL);

    //Create area-exposed context memory struct
    /*
        This only works because gtk_main keeps this stack alive.
        If this function were to return, this memory would no longer be available.
        In such case, malloc is required.
    */
    struct GifTimerInput gin ; 
    gin.image = gtk_image_new();

    //Create loader, register signal and write gif binary
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new ();
    g_signal_connect (G_OBJECT (loader), "area-prepared", G_CALLBACK (gif_area_prepared), &gin);
    gdk_pixbuf_loader_write (loader, (const guchar *)_binary_loading_gif_start, _binary_loading_gif_end - _binary_loading_gif_start, NULL);

    //Create handle containing the image
    GtkWidget * handle = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (handle), gin.image);

    //Optionally set the background
    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black;}",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssProvider); 

    gtk_widget_set_app_paintable (GTK_WIDGET (handle), TRUE);
    
    gtk_container_add (GTK_CONTAINER (window), handle);
    
    /* Run the program */
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}