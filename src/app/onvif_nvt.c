#include "onvif_nvt.h"
#include "gtkstyledimage.h"
#include "gui_utils.h"
#include "clogger.h"
#include "../gst/gstrtspstream.h"
#include "onvif_nvt_stream_details.h"

extern char _binary_microphone_png_size[];
extern char _binary_microphone_png_start[];
extern char _binary_microphone_png_end[];

extern char _binary_info_png_size[];
extern char _binary_info_png_start[];
extern char _binary_info_png_end[];

enum {
  PROP_PLAYER = 1,
  N_PROPERTIES
};

typedef struct {
    GstRtspPlayer * player;
    GtkWidget * info_panel;
    GtkWidget * overlay_grid;
    GtkWidget * streams_info_container;
} OnvifNVTPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(OnvifNVT, OnvifNVT_, GTK_TYPE_OVERLAY)

static gboolean 
toggle_mic_release_cb (GtkWidget *widget, gpointer * p, GstRtspPlayer * player){
    GstRtspPlayer__mic_mute(player,TRUE);
    return FALSE;
}

static gboolean 
toggle_mic_press_cb (GtkWidget *widget, gpointer * p, GstRtspPlayer * player){
    GstRtspPlayer__mic_mute(player,FALSE);
    return FALSE;
}

static void 
OnvifNVT__player_stopped_cb(GstRtspPlayer * player, OnvifNVT * self){
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    gtk_widget_set_visible(priv->overlay_grid,FALSE);
    gtk_container_foreach(GTK_CONTAINER(priv->streams_info_container),(GtkCallback)gui_widget_destroy, NULL);
}

static void
OnvifNVT__player_notplaying_cb(GstRtspPlayer * player, GstRtspPlayerSession * session, OnvifNVT * self){
    OnvifNVT__player_stopped_cb(player, self);
    //TODO Display message
}

static void 
OnvifNVT__player_started_cb(GstRtspPlayer * player, GstRtspPlayerSession * session, OnvifNVT * self){
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    gtk_widget_set_visible(priv->overlay_grid,TRUE);
}

static void
OnvifNVT__info_toggled_cb(GtkToggleButton * btn, OnvifNVT * self){
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    gtk_widget_set_visible(priv->info_panel,gtk_toggle_button_get_active(btn));
}

static void
OnvifNVT__player_new_stream_cb(GstRtspPlayer * player, GstRtspStream * stream, OnvifNVT * self){
    GtkWidget * stream_panel =  OnvifNVTStreamDetails__new(stream);
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    gtk_box_pack_start (GTK_BOX (priv->streams_info_container), stream_panel, FALSE, TRUE, 5);
    gtk_widget_show_all(priv->streams_info_container);
}

static GtkWidget * 
OnvifNVT__create_controls_overlay(OnvifNVT * self){ 
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    GtkWidget * controls = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    priv->overlay_grid = gtk_grid_new ();
    gtk_grid_attach (GTK_GRID (priv->overlay_grid), controls, 0, 1, 1, 1);

    g_signal_connect (G_OBJECT(priv->player), "retry", G_CALLBACK (OnvifNVT__player_notplaying_cb), self);
    g_signal_connect (G_OBJECT(priv->player), "error", G_CALLBACK (OnvifNVT__player_notplaying_cb), self);
    g_signal_connect (G_OBJECT(priv->player), "stopped", G_CALLBACK (OnvifNVT__player_stopped_cb), self);
    g_signal_connect (G_OBJECT(priv->player), "started", G_CALLBACK (OnvifNVT__player_started_cb), self);

    gtk_widget_set_margin_top(controls,10);
    gtk_widget_set_margin_start(controls,5);
    gtk_widget_set_margin_bottom(controls,10);

    GtkWidget * image = GtkStyledImage__new((unsigned char *)_binary_microphone_png_start, _binary_microphone_png_end - _binary_microphone_png_start, 20, 20, NULL);
    GtkWidget * widget = gtk_button_new ();
    g_signal_connect (widget, "button-press-event", G_CALLBACK (toggle_mic_press_cb), priv->player);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (toggle_mic_release_cb), priv->player);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_box_pack_start (GTK_BOX (controls), widget, FALSE, FALSE, 5);

    image = GtkStyledImage__new((unsigned char *)_binary_info_png_start, _binary_info_png_end - _binary_info_png_start, 20, 20, NULL);
    widget = gtk_toggle_button_new (); 

    /*
        Some themes like BreezeDark will use Alpha:0 color for checked state.
        This doesn't look great over a non-plain background like a video stream
        Instead, simply inverting default theme for checked state
    */
    gui_widget_set_css(widget,"button.toggle:checked { background: @theme_fg_color; color: @theme_bg_color; }");

    g_signal_connect (widget, "toggled", G_CALLBACK (OnvifNVT__info_toggled_cb), self);
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(widget),TRUE);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_box_pack_start (GTK_BOX (controls), widget, FALSE, FALSE, 5);

    priv->info_panel = gtk_frame_new(NULL);
    gtk_widget_set_margin_start(priv->info_panel,10);
    //Slight transparence for the detail panel so that it's readable, while still allowing to view the stream
    gui_widget_set_css(priv->info_panel,"* { border-color:grey; border-radius: 10px; background-color: rgba(0, 0, 0, 0.25); }");
    gtk_grid_attach (GTK_GRID (priv->overlay_grid), priv->info_panel, 0, 2, 1, 1);

    GtkWidget * frame_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_widget_set_margin_start(frame_panel,10);
    gtk_widget_set_margin_end(frame_panel,10);
    gtk_widget_set_margin_top(frame_panel,10);
    gtk_widget_set_margin_bottom(frame_panel,10);
    gtk_container_add(GTK_CONTAINER(priv->info_panel),frame_panel);

    GtkWidget * label = gtk_label_new("Stream Information");
    gtk_widget_set_halign(label,GTK_ALIGN_START);
    gtk_box_pack_start (GTK_BOX (frame_panel), label, FALSE, FALSE, 0);

    priv->streams_info_container = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_widget_set_margin_start(priv->streams_info_container ,10);
    gtk_box_pack_start (GTK_BOX (frame_panel), priv->streams_info_container, FALSE, FALSE, 0);

    //Show all childs, hide root and take manual visibility control
    gtk_widget_show_all(priv->overlay_grid);
    gtk_widget_set_visible(priv->overlay_grid,FALSE);
    gtk_widget_set_visible(priv->info_panel,FALSE);
    gtk_widget_set_no_show_all(priv->overlay_grid, TRUE);
    return priv->overlay_grid;
}

static void
OnvifNVT__create_ui (OnvifNVT * self){
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    GtkWidget *grid;
    GtkWidget *widget;

    //some container is required so that there's something to paint once the player is hidden
    grid = gtk_grid_new ();
    widget = GstRtspPlayer__createCanvas(priv->player);
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_widget_set_hexpand (widget, TRUE);

    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

    //Default black stream background
    gui_widget_set_css(grid,"* { background-image:none; background-color:black;}");

    gtk_container_add (GTK_CONTAINER (self), grid);

    widget = OnvifNVT__create_controls_overlay(self);

    gtk_overlay_add_overlay(GTK_OVERLAY(self),widget);
}

static void
OnvifNVT__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifNVT * self = ONVIFMGR_NVT (object);
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);

    switch (prop_id){
        case PROP_PLAYER:
            priv->player = g_value_get_object (value);
            g_signal_connect (G_OBJECT(priv->player), "new-stream", G_CALLBACK (OnvifNVT__player_new_stream_cb), self);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifNVT__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifNVT *thread = ONVIFMGR_NVT (object);
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (thread);
    switch (prop_id){
        case PROP_PLAYER:
            g_value_set_object (value, priv->player);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifDetailsPanel__constructed(GObject* object){
    OnvifNVT__create_ui(ONVIFMGR_NVT(object));
    G_OBJECT_CLASS (OnvifNVT__parent_class)->constructed (object);
}

static void
OnvifNVT__class_init (OnvifNVTClass * klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifNVT__set_property;
    object_class->get_property = OnvifNVT__get_property;
    object_class->constructed = OnvifDetailsPanel__constructed;

    obj_properties[PROP_PLAYER] =
    g_param_spec_object ("player",
                        "GstRtspPlayer",
                        "The GstRtspPlayer to wrap and add control to.",
                        GST_TYPE_RTSPPLAYER,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                      N_PROPERTIES,
                                      obj_properties);
}

static void
OnvifNVT__init (OnvifNVT * self){
    OnvifNVTPrivate *priv = OnvifNVT__get_instance_private (self);
    priv->info_panel = NULL;
    priv->overlay_grid = NULL;
    priv->player = NULL;
    priv->streams_info_container = NULL;
}

GtkWidget* 
OnvifNVT__new(GstRtspPlayer * player){
    return g_object_new (ONVIFMGR_TYPE_NVT, "player", player, NULL);
}
