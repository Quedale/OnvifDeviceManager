#include "gtkprofilepanel.h"
#include <gtk/gtk.h>
#include "clogger.h"

typedef struct {
    OnvifMediaProfile * profile;
} GtkProfilePanelPrivate;

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GtkProfilePanel, gtk_profile_panel, GTK_TYPE_BOX)

static void 
gtk_profile_panel_init (GtkProfilePanel *widget)
{
    GtkProfilePanelPrivate *priv = gtk_profile_panel_get_instance_private (widget);
    priv->profile = NULL;
}

static void 
gtk_profile_panel_class_init (GtkProfilePanelClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    button_signals[CLICKED] =
        g_signal_new ("clicked",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (GtkProfilePanelClass, clicked),
                NULL, NULL,
                NULL,
                G_TYPE_NONE, 0);

    gtk_widget_class_set_css_name (widget_class, "profilebutton");

}

OnvifMediaProfile * 
gtk_profile_panel_get_profile (GtkProfilePanel *widget)
{
    GtkProfilePanelPrivate *priv = gtk_profile_panel_get_instance_private (widget);

    g_return_val_if_fail (GTK_IS_PROFILE_PANEL (widget), 0);

    return priv->profile;
}

void gtk_profile_panel_clicked (GtkButton* widget, GtkProfilePanel * pbutton){
    g_signal_emit (pbutton, button_signals[CLICKED], 0);
}

void
gtk_profile_panel_set_profile (GtkProfilePanel * widget, OnvifMediaProfile * profile)
{
    GtkProfilePanelPrivate *priv = gtk_profile_panel_get_instance_private (widget);

    g_return_if_fail (GTK_IS_PROFILE_PANEL (widget));

    priv->profile = profile;

    GtkWidget * btn = gtk_button_new_with_label(OnvifMediaProfile__get_name(profile));
    gtk_box_pack_start(GTK_BOX(widget), btn,     FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT(btn), "clicked", G_CALLBACK (gtk_profile_panel_clicked), widget);
}


GtkWidget*  gtk_profile_panel_new (OnvifMediaProfile * profile){
    GtkWidget * widget = g_object_new (GTK_TYPE_PROFILE_PANEL, 
                        "orientation", GTK_ORIENTATION_HORIZONTAL,
                        "spacing", 0,
                        NULL);
    gtk_profile_panel_set_profile(GTK_PROFILE_PANEL(widget),profile);
    return widget;
}