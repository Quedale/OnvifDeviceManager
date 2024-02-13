#include "onvif_nvt.h"

extern char _binary_microphone_png_size[];
extern char _binary_microphone_png_start[];
extern char _binary_microphone_png_end[];


gboolean toggle_mic_cb (GtkWidget *widget, gpointer * p, gpointer * p2){
    RtspPlayer * player = (RtspPlayer *) p2;
    if(RtspPlayer__is_mic_mute(player)){
        RtspPlayer__mic_mute(player,FALSE);
    } else {
        RtspPlayer__mic_mute(player,TRUE);
    }
    return FALSE;
}

GtkWidget * create_controls_overlay(RtspPlayer *player){

    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
    gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_microphone_png_start, _binary_microphone_png_end - _binary_microphone_png_start, NULL);

    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    double ph = gdk_pixbuf_get_height (pixbuf);
    double pw = gdk_pixbuf_get_width (pixbuf);
    double newpw = 30 / ph * pw;
    pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,30,GDK_INTERP_NEAREST);
    GtkWidget * image = gtk_image_new_from_pixbuf (pixbuf); 

    GtkWidget * widget = gtk_button_new ();
    g_signal_connect (widget, "button-press-event", G_CALLBACK (toggle_mic_cb), player);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (toggle_mic_cb), player);

    gtk_button_set_image (GTK_BUTTON (widget), image);

    GtkWidget * fixed = gtk_fixed_new();
    gtk_fixed_put(GTK_FIXED(fixed),widget,10,10); 

    g_object_unref(pixbuf);
    gdk_pixbuf_loader_close(loader,NULL);
    g_object_unref(loader);

    return fixed;
}

GtkWidget * OnvifNVT__create_ui (RtspPlayer * player){
    GtkWidget *grid;
    GtkWidget *widget;

    grid = gtk_grid_new ();
    widget = RtspPlayer__createCanvas(player);
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_widget_set_hexpand (widget, TRUE);

    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; background-color:black;}",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(grid);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssProvider);  

    GtkWidget * overlay =gtk_overlay_new();
    gtk_container_add (GTK_CONTAINER (overlay), grid);

    widget = create_controls_overlay(player);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay),widget);
    return overlay;
}

