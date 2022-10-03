#include "player.h"

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  GError *err;
  gchar *debug_info;

  /* Print error details on the screen */
  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);

  /* Set the pipeline to READY (which stops playback) */
  gst_element_set_state (data->playbin, GST_STATE_READY);
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, OnvifPlayer *data) {
  g_print ("End-Of-Stream reached.\n");
  gst_element_set_state (data->playbin, GST_STATE_READY);
}

/* This function is called when the GUI toolkit creates the physical window that will hold the video.
 * At this point we can retrieve its handler (which has a different meaning depending on the windowing system)
 * and pass it to GStreamer through the VideoOverlay interface. */
static void realize_cb (GtkWidget *widget, OnvifPlayer *data) {
  GdkWindow *window = gtk_widget_get_window (widget);
  guintptr window_handle;

  if (!gdk_window_ensure_native (window))
    g_error ("Couldn't create native window needed for GstVideoOverlay!");

  /* Retrieve window handler from GDK */
#if defined (GDK_WINDOWING_WIN32)
  window_handle = (guintptr)GDK_WINDOW_HWND (window);
#elif defined (GDK_WINDOWING_QUARTZ)
  window_handle = gdk_quartz_window_get_nsview (window);
#elif defined (GDK_WINDOWING_X11)
  window_handle = GDK_WINDOW_XID (window);
#endif
  /* Pass it to playbin, which implements VideoOverlay and will forward it to the video sink */
  gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->playbin), window_handle);
}

/* This function is called everytime the video window needs to be redrawn (due to damage/exposure,
 * rescaling, etc). GStreamer takes care of this in the PAUSED and PLAYING states, otherwise,
 * we simply draw a black rectangle to avoid garbage showing up. */
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, OnvifPlayer *data) {
  if (data->state < GST_STATE_PAUSED) {
    GtkAllocation allocation;

    /* Cairo is a 2D graphics library which we use here to clean the video window.
     * It is used by GStreamer for other reasons, so it will always be available to us. */
    gtk_widget_get_allocation (widget, &allocation);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    cairo_fill (cr);
  }

  return FALSE;
}

void OnvifPlayer__init(OnvifPlayer* self) {

    GstBus *bus;

    self->onvifDeviceList = OnvifDeviceList__create();

    /* Create the elements */
    self->playbin = gst_element_factory_make ("playbin", "playbin");

    if (!self->playbin) {
        g_printerr ("Not all elements could be created.\n");
        return;
    }

    // g_signal_connect (G_OBJECT (self->playbin), "video-tags-changed", (GCallback) tags_cb, &self);
    // g_signal_connect (G_OBJECT (self->playbin), "audio-tags-changed", (GCallback) tags_cb, &self);
    // g_signal_connect (G_OBJECT (self->playbin), "text-tags-changed", (GCallback) tags_cb, &self);

    // /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus (self->playbin);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, &self);
    g_signal_connect (G_OBJECT (bus), "message::eos", (GCallback)eos_cb, &self);
    // g_signal_connect (G_OBJECT (bus), "message::state-changed", (GCallback)state_changed_cb, &self);
    gst_object_unref (bus);
}

OnvifPlayer OnvifPlayer__create() {
    OnvifPlayer result;
    memset (&result, 0, sizeof (result));
    OnvifPlayer__init(&result);
    return result;
}

void OnvifPlayer__reset(OnvifPlayer* self) {
}

void OnvifPlayer__destroy(OnvifPlayer* self) {
  if (self) {
     OnvifPlayer__reset(self);
     free(self);
  }
}

void OnvifPlayer__set_playback_url(OnvifPlayer* self, char *url) {
    g_object_set (self->playbin, "uri", url, NULL);
}

void OnvifPlayer__play(OnvifPlayer* self){
    GstStateChangeReturn ret;
    ret = gst_element_set_state (self->playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (self->playbin);
        return;
    }
    printf("set state to playing..\n");
}

void OnvifPlayer__set_canvas(OnvifPlayer* self, GtkWidget* canvas){
    g_signal_connect (canvas, "realize", G_CALLBACK (realize_cb), self);
    g_signal_connect (canvas, "draw", G_CALLBACK (draw_cb), self);
}