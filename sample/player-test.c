#include "../src/gst/player.h"
#include "../src/gst/onvifinitstaticplugins.h"

int main(int argc, char *argv[])
{
    /* Initialize GTK */
    gtk_init (&argc, &argv);

    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    onvif_init_static_plugins();


    RtspPlayer * player = RtspPlayer__create();
    RtspPlayer__set_playback_url(player,"rtsp://192.168.1.10:8664/video0");
    RtspPlayer__play(player);

    /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
    gtk_main ();


    gst_deinit ();
}