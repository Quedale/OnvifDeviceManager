#include "app_settings.h"

#define CONFIG_FILE_PATH "onvifmgr_settings.ini"

typedef struct _AppSettings {
    //Generic settings widgets
    GtkWidget * widget;
    GtkWidget * loading_handle;
    GtkWidget * notice;
    GtkWidget * apply_button;
    GtkWidget * reset_btn;

    //Stream overscaling specific properties
    GtkWidget * overscale_chk;
    int allow_overscale;
    void (*overscale_callback)(AppSettings *, int, void *);
    void * overscale_userdata;

    EventQueue * queue;
} AppSettings;

typedef struct {
    AppSettings * settings;
    int allow_overscale_changed;
    //More settings flags here
} AppSettingChanges;

void set_settings_state(AppSettings * settings, int state){
    gtk_widget_set_sensitive(settings->overscale_chk,state);
    //More settings to add here
}

void set_button_state(AppSettings * settings, int state){
    gtk_widget_set_visible(settings->notice,state);
    gtk_widget_set_sensitive(settings->apply_button,state);
    gtk_widget_set_sensitive(settings->reset_btn,state);
}

void check_button_state (AppSettings * settings){
    int scale_val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(settings->overscale_chk));
    if(scale_val != settings->allow_overscale){
        set_button_state(settings,TRUE);
    } else {
        set_button_state(settings,FALSE);
    }
    //More settings widgets here
}

//Generic value callback for togglebuttons
void value_toggled (GtkToggleButton* self, AppSettings * settings){
    check_button_state(settings);
}

//GUI callback when apply finished
void save_done(void * user_data){
    AppSettings * settings = (AppSettings *) user_data;
    check_button_state(settings);
    set_settings_state(settings,TRUE);
    gtk_spinner_stop (GTK_SPINNER (settings->loading_handle));
}

//Background task to save settings (invokes internal callbacks)
void _save_settings(void * user_data){
    AppSettingChanges * changes = (AppSettingChanges *) user_data;

    if(changes->allow_overscale_changed && changes->settings->overscale_callback){
        changes->settings->overscale_callback(changes->settings, changes->settings->allow_overscale, changes->settings->overscale_userdata);
    }
    //More settings callbacks to add here

    FILE * fptr = fopen(CONFIG_FILE_PATH,"w");
    if(fptr != NULL){
        if(changes->settings->allow_overscale){
            fprintf(fptr,"allow_overscaling=true");
        } else {
            fprintf(fptr,"allow_overscaling=false");
        }
        //More settings to save here
        
        fclose(fptr);
    } else {
        printf("ERROR wrtting to settings file!\n");
    }

    gdk_threads_add_idle((void *)save_done,changes->settings);

    free(changes);
}

// Apply button GUI callback
void apply_settings (GtkWidget *widget, AppSettings * settings) {
    set_settings_state(settings,FALSE);
    set_button_state(settings,FALSE);
    gtk_spinner_start (GTK_SPINNER (settings->loading_handle));
    AppSettingChanges * changes = malloc(sizeof(AppSettingChanges));
    changes->settings = settings;

    int new_overscale = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(settings->overscale_chk));
    changes->allow_overscale_changed = new_overscale != changes->settings->allow_overscale;
    changes->settings->allow_overscale = new_overscale;

    EventQueue__insert(settings->queue,_save_settings,changes);
}

void AppSettings__reset_settings(AppSettings * settings){
    if(settings->allow_overscale){
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (settings->overscale_chk),TRUE);
    } else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (settings->overscale_chk),FALSE);
    }
}

void reset_settings (GtkWidget *widget, AppSettings * settings) {
    AppSettings__reset_settings(settings);
}

void AppSettings__set_overscale_callback(AppSettings * self, void (*overscale_callback)(AppSettings *, int, void * ), void * overscale_userdata){
    self->overscale_callback = overscale_callback;
    self->overscale_userdata = overscale_userdata;
}

void AppSettings__create_ui(AppSettings * self){
    GtkWidget * widget;
    GtkWidget * label;

    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    self->widget = gtk_grid_new();
    g_object_set (self->widget, "margin", 10, NULL);
    gtk_widget_set_hexpand (self->widget, TRUE);

    self->overscale_chk = gtk_check_button_new_with_label("Allow stream overscale.");

    gtk_grid_attach (GTK_GRID (self->widget), self->overscale_chk, 0, 1, 1, 1);

    //Add spacer to align button bar at the button
    widget = gtk_label_new("");
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_grid_attach (GTK_GRID (self->widget), widget, 0, 2, 1, 1);

    //Create button bar
    GtkWidget * button_bar = gtk_grid_new();

    self->notice = gtk_frame_new("");
    gtk_widget_set_no_show_all(self->notice,TRUE);
    gtk_widget_set_visible(self->notice,FALSE);
    g_object_set (self->notice, "margin", 10, NULL);

    widget = gtk_label_new("Don't forget to apply your settings.");
    g_object_set (widget, "margin-bottom", 10, NULL);
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_CENTER);
    gtk_widget_show(widget);

    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; color:#DE5E09;}",-1,NULL); 
    context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssProvider);  

    cssProvider = gtk_css_provider_new();
    
    gtk_css_provider_load_from_data(cssProvider, "* { background-image:none; border-color:#FAB05B; border-radius:16px}",-1,NULL); 
    context = gtk_widget_get_style_context(self->notice);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref (cssProvider);  

    gtk_container_add(GTK_CONTAINER(self->notice),widget);
    
    gtk_grid_attach (GTK_GRID (button_bar), self->notice, 0, 0, 3, 1);
    
    //Create Apply button
    self->apply_button = gtk_button_new ();
    gtk_widget_set_hexpand (self->apply_button, TRUE);
    gtk_widget_set_sensitive(self->apply_button,FALSE);
    label = gtk_label_new("Apply");
    g_object_set (self->apply_button, "margin-end", 10, NULL);
    gtk_container_add (GTK_CONTAINER (self->apply_button), label);
    gtk_widget_set_opacity(self->apply_button,1);
    g_signal_connect (self->apply_button, "clicked", G_CALLBACK (apply_settings), self);
    gtk_grid_attach (GTK_GRID (button_bar), self->apply_button, 0, 1, 1, 1);
    
    //Create reset button
    self->reset_btn = gtk_button_new ();
    gtk_widget_set_hexpand (self->reset_btn, TRUE);
    gtk_widget_set_sensitive(self->reset_btn,FALSE);
    label = gtk_label_new("Reset");
    gtk_container_add (GTK_CONTAINER (self->reset_btn), label);
    gtk_widget_set_opacity(self->reset_btn,1);
    g_signal_connect (self->reset_btn, "clicked", G_CALLBACK (reset_settings), self);
    gtk_grid_attach (GTK_GRID (button_bar), self->reset_btn, 1, 1, 1, 1);

    //TODO Support force reload config file
    // widget = gtk_button_new ();
    // gtk_widget_set_hexpand (widget, TRUE);
    // label = gtk_label_new("Reload");
    // g_object_set (widget, "margin-start", 10, NULL);
    // gtk_container_add (GTK_CONTAINER (widget), label);
    // gtk_widget_set_opacity(widget,1);
    // g_signal_connect (widget, "clicked", G_CALLBACK (apply_settings), self);
    // gtk_grid_attach (GTK_GRID (button_bar), widget, 2, 1, 1, 1);

    gtk_grid_attach (GTK_GRID (self->widget), button_bar, 0, 3, 1, 1);

    g_signal_connect (G_OBJECT (self->overscale_chk), "toggled", G_CALLBACK (value_toggled), self);
}

#define MAX_LEN 256

void AppSettings__load_settings(AppSettings * self){
    FILE *fptr = NULL;
    
    printf("Reading Settings file %s\n",CONFIG_FILE_PATH);
    if (access(CONFIG_FILE_PATH, F_OK) == 0) {
        fptr = fopen(CONFIG_FILE_PATH,"r");
        if(fptr != NULL){
            char buffer[MAX_LEN];
            while (fgets(buffer, MAX_LEN, fptr)){
                // Remove trailing newline
                buffer[strcspn(buffer, "\n")] = 0;

                char * buff_ptr = (char*)buffer;
                char * key = strtok_r (buff_ptr, "=", &buff_ptr);
                char * val = strtok_r (buff_ptr, "\n", &buff_ptr); //Get reminder of line
                printf("\t'%s' = '%s'\n",key,val);

                if(!strcmp(key,"allow_overscaling")){
                    if(!strcmp(val,"false")){
                        self->allow_overscale = 0;
                    }
                }
            }
        } else {
            printf("WARNING no config file found. Using default configs. 2\n");
        }
    } else {
        printf("WARNING no config file found. Using default configs. 1\n");
    }

    //Force defaults
    if(self->allow_overscale == -1){
        self->allow_overscale = 1;
    }

    if(fptr)
        fclose(fptr);
}

AppSettings * AppSettings__create(EventQueue * queue){
    AppSettings * self = malloc(sizeof(AppSettings));
    self->queue = queue;
    self->allow_overscale = -1;
    AppSettings__load_settings(self);
    AppSettings__create_ui(self);
    AppSettings__reset_settings(self);
    check_button_state(self);
    return self;
}

void AppSettings__destroy(AppSettings* self){
    if(self){
        free(self);
    }
}

void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget){
    self->loading_handle = widget;
}

GtkWidget * AppSettings__get_widget(AppSettings * self){
    return self->widget;
}

int AppSettings__get_allow_overscale(AppSettings * self){
    return self->allow_overscale;
}