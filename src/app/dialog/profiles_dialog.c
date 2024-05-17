#include "profiles_dialog.h"
#include "clogger.h"
#include "../gui_utils.h"
#include "app_dialog.h"
#include "../../animations/gtk/gtk_dotted_slider_widget.h"
#include "gtkprofilepanel.h"


typedef struct {
    GtkWidget * primary_pane;
    GtkWidget * content_pane;
} DialogElements;

typedef struct {
    ProfilesDialog * dialog;
    OnvifProfiles * profiles;
} GUIProfilesEvent;

typedef struct {
    ProfilesDialog * dialog;
    OnvifProfile * profiles;
} GUIProfileEvent;

GtkWidget * priv_ProfilesDialog__create_ui(AppDialogEvent * event){
    C_TRACE("priv_ProfilesDialog__create_ui");
    ProfilesDialog * dialog = (ProfilesDialog *) event->dialog; 
    DialogElements * elements = (DialogElements *) dialog->elements;

    elements->primary_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    return elements->primary_pane;
}

void priv_ProfilesDialog__destroy(AppDialog * dialog){
    ProfilesDialog* cdialog = (ProfilesDialog*)dialog;
    free(cdialog->elements);
}

static void ProfilesDialog_profile_clicked (GtkProfilePanel * button, ProfilesDialog * dialog) {
    (*(dialog->profile_selected))(dialog, gtk_profile_panel_get_profile(button));
    AppDialog__hide((AppDialog*)dialog);
}

gboolean gui_ProfilesDialog__show_profiles(void * user_data){
    GUIProfilesEvent * evt = (GUIProfilesEvent *) user_data;
    DialogElements * elements = (DialogElements *) evt->dialog->elements;

    gtk_widget_destroy(elements->content_pane);
    elements->content_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    for(int i=0;i<OnvifProfiles__get_size(evt->profiles);i++){
        OnvifProfile * profile = OnvifProfiles__get_profile(evt->profiles,i);

        GtkWidget * btn = gtk_profile_panel_new(profile);
        gtk_box_pack_start(GTK_BOX(elements->content_pane), btn,     FALSE, FALSE, 0);
        g_signal_connect (G_OBJECT(btn), "clicked", G_CALLBACK (ProfilesDialog_profile_clicked), evt->dialog);
    }

    gtk_box_pack_start(GTK_BOX(elements->primary_pane), elements->content_pane,     FALSE, FALSE, 0);
    gtk_widget_show_all (elements->primary_pane);
    
    free(evt);
    return FALSE;
}

void _priv_ProfilesDialog__load_profiles(void * user_data){
    ProfilesDialog* cdialog = (ProfilesDialog*)user_data;
    OnvifMediaService * mserv = OnvifDevice__get_media_service(OnvifMgrDeviceRow__get_device(cdialog->device));
    OnvifProfiles * profiles = OnvifMediaService__get_profiles(mserv);

    GUIProfilesEvent * evt = malloc(sizeof(GUIProfilesEvent));
    evt->dialog = cdialog;
    evt->profiles= profiles;
    gdk_threads_add_idle(G_SOURCE_FUNC(gui_ProfilesDialog__show_profiles),evt);

}

void priv_ProfilesDialog__show_cb(AppDialogEvent * event){
    C_DEBUG("priv_ProfilesDialog__show_cb");
    ProfilesDialog * dialog = (ProfilesDialog *) event->dialog;
    DialogElements * elements = (DialogElements *) dialog->elements;

    //Clear previous content
    if(GTK_IS_WIDGET(elements->content_pane)) gtk_widget_destroy(elements->content_pane);

    //Show global loading
    elements->content_pane = gtk_grid_new();
    GtkWidget * empty = gtk_label_new("Loading device profiles...");
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (elements->content_pane), empty, 0, 0, 1, 1);


    GtkWidget * slider = gtk_dotted_slider_new(GTK_ORIENTATION_HORIZONTAL, 5,10,1);
    gtk_widget_set_halign(slider, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(slider, GTK_ALIGN_CENTER);
    gtk_grid_attach (GTK_GRID (elements->content_pane), slider, 0, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(elements->primary_pane), elements->content_pane,     TRUE, FALSE, 0);

    EventQueue__insert_plain(dialog->queue, dialog->device, _priv_ProfilesDialog__load_profiles,dialog, NULL);
}

void ProfilesDialog__set_device(ProfilesDialog * self, OnvifMgrDeviceRow * device){
    self->device = device;
}

OnvifMgrDeviceRow * ProfilesDialog__get_device(ProfilesDialog * self){
    return self->device;
}

ProfilesDialog * ProfilesDialog__create(EventQueue * queue, void (* clicked)  (ProfilesDialog * dialog, OnvifProfile * profile)){
    ProfilesDialog * dialog = malloc(sizeof(ProfilesDialog));
    DialogElements * elements = malloc(sizeof(DialogElements));
    elements->content_pane = NULL;

    dialog->elements = elements;
    dialog->queue = queue;
    dialog->device = NULL;
    dialog->profile_selected = clicked;
    C_TRACE("create");
    AppDialog__init((AppDialog *)dialog, priv_ProfilesDialog__create_ui);
    CObject__set_allocated((CObject *) dialog);
    AppDialog__set_destroy_callback((AppDialog*)dialog,priv_ProfilesDialog__destroy);
    AppDialog__set_title((AppDialog*)dialog,"Profile Selector");
    AppDialog__set_submitable((AppDialog*)dialog,0);
    AppDialog__set_show_callback((AppDialog *)dialog,priv_ProfilesDialog__show_cb);

    return dialog;
}