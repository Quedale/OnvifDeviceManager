#ifndef gtk_profile_panel_H_ 
#define gtk_profile_panel_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>
#include "../device.h"

G_BEGIN_DECLS

typedef struct _GtkProfilePanel        GtkProfilePanel;
typedef struct _GtkProfilePanelClass   GtkProfilePanelClass;

#define GTK_TYPE_PROFILE_PANEL (gtk_profile_panel_get_type ())
#define GTK_PROFILE_PANEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PROFILE_PANEL, GtkProfilePanel))//(obj)          GTK_CHECK_CAST (obj, gtk_profile_panel_get_type (), GtkProfilePanel)
#define GTK_PROFILE_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PROFILE_PANEL, GtkProfilePanelClass)) //(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_profile_panel_get_type (), GtkProfilePanelClass)
#define GTK_IS_PROFILE_PANEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PROFILE_PANEL)) //      GTK_CHECK_TYPE (obj, gtk_profile_panel_get_type ())
#define GTK_IS_PROFILE_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PROFILE_PANEL))
#define GTK_PROFILE_PANEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PROFILE_PANEL, GtkProfilePanelClass))

struct _GtkProfilePanel
{
    GtkBox parent;
};

struct _GtkProfilePanelClass
{
    void (* clicked)  (GtkProfilePanel * dialog, OnvifProfile * profile);
    GtkBoxClass parent_class;

};

GType          gtk_profile_panel_get_type        (void);
GtkWidget*     gtk_profile_panel_new             (OnvifProfile * profile);
void           gtk_profile_panel_set_profile (GtkProfilePanel *widget, OnvifProfile * profile);
OnvifProfile * gtk_profile_panel_get_profile (GtkProfilePanel *widget);

G_END_DECLS

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif