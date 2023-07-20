#include "app_settings.h"

#define CONFIG_FILE_PATH "onvifmgr_settings.ini"


void set_settings_state(AppSettings * settings, int state){
    AppSettingsStream__set_state(settings->stream, state);
    //More settings to add here
}

void set_button_state(AppSettings * settings, int state){
    gtk_widget_set_visible(settings->notice,state);
    gtk_widget_set_sensitive(settings->apply_button,state);
    gtk_widget_set_sensitive(settings->reset_btn,state);
}

void priv_AppSettings_state_changed(void * data){
    AppSettings * self = (AppSettings*) data;
    //More settings to add here
    if(AppSettingsStream__get_state(self->stream)){
        set_button_state(self,TRUE);
    } else {
        set_button_state(self,FALSE);
    }
}

//GUI callback when apply finished
void save_done(void * user_data){
    AppSettings * settings = (AppSettings *) user_data;
    priv_AppSettings_state_changed(settings);
    set_settings_state(settings,TRUE);
    gtk_spinner_stop (GTK_SPINNER (settings->loading_handle));
}

//Background task to save settings (invokes internal callbacks)
void _save_settings(void * user_data){
    AppSettings * self = (AppSettings *) user_data;

    char * stream_settings_txt = AppSettingsStream__save(self->stream);
    //More settings to add here

    FILE * fptr = fopen(CONFIG_FILE_PATH,"w");
    if(fptr != NULL){
        fprintf(fptr,"%s",stream_settings_txt);
        //More settings to add here
        
        fclose(fptr);
    } else {
        printf("ERROR wrtting to settings file!\n");
    }

    gdk_threads_add_idle((void *)save_done,self);

}

// Apply button GUI callback
void apply_settings (GtkWidget *widget, AppSettings * settings) {
    set_settings_state(settings,FALSE);
    set_button_state(settings,FALSE);
    gtk_spinner_start (GTK_SPINNER (settings->loading_handle));

    EventQueue__insert(settings->queue,_save_settings,settings);
}

void AppSettings__reset_settings(AppSettings * settings){
    AppSettingsStream__reset(settings->stream);
    //More settings to add here
}

void reset_settings (GtkWidget *widget, AppSettings * settings) {
    AppSettings__reset_settings(settings);
    //More settings to add here
}

void AppSettings__create_ui(AppSettings * self){
    GtkWidget * widget;
    GtkWidget * notebook;
    GtkWidget * label;

    GtkCssProvider * cssProvider;
    GtkStyleContext * context;

    self->widget = gtk_grid_new(); //Create root setting container

    notebook = gtk_notebook_new(); //Create note containing setting pages
    gtk_widget_set_vexpand (notebook, TRUE);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);
    gtk_grid_attach (GTK_GRID (self->widget), notebook, 0, 0, 1, 1);


    widget = AppSettingsStream__get_widget(self->stream);
    //Add stream page to notebook
    label = gtk_label_new ("Stream");
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widget, label);

    //More settings to add here

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

    gtk_grid_attach (GTK_GRID (self->widget), button_bar, 0, 2, 1, 1);
}

#define MAX_LEN 256

void AppSettings__load_settings(AppSettings * self){
    FILE *fptr = NULL;
    
    AppSettingsType category = -1;
    
    printf("Reading Settings file %s\n",CONFIG_FILE_PATH);
    if (access(CONFIG_FILE_PATH, F_OK) == 0) {
        fptr = fopen(CONFIG_FILE_PATH,"r");
        if(fptr != NULL){
            char buffer[MAX_LEN];
            while (fgets(buffer, MAX_LEN, fptr)){
                // Remove trailing newline
                buffer[strcspn(buffer, "\n")] = 0;

                if(buffer[0] == '['){
                    printf("Parsing category %s\n",buffer);
                    int newlen = strlen(buffer)-2;
                    char cat[newlen];
                    strncpy(cat,buffer + 1,newlen);
                    if(AppSettingsStream__is_category(self->stream,cat)){
                        category = APPSETTING_STREAM_TYPE;
                    }//More settings to add here

                    continue;
                } else if(category != -1){
                    char * buff_ptr = (char*)buffer;
                    char * key = strtok_r (buff_ptr, "=", &buff_ptr);
                    char * val = strtok_r (buff_ptr, "\n", &buff_ptr); //Get reminder of line
                    printf("\t Property : '%s' = '%s'\n",key,val);

                    switch(category){
                        case APPSETTING_STREAM_TYPE:
                            if(!AppSettingsStream__set_property(self->stream,key,val)){
                                printf("WARNING: Unknown stream property %s=%s\n",key,val);
                            }//More settings to add here
                            break;
                        default:
                            //TODO Warning
                            break;
                    }
                    continue;
                } else {
                    printf("WARNING Setting without category\n");
                }
            }
        } else {
            printf("WARNING no config file found. Using default configs. 2\n");
        }
    } else {
        printf("WARNING no config file found. Using default configs. 1\n");
    }

    if(fptr)
        fclose(fptr);
}

AppSettings * AppSettings__create(EventQueue * queue){
    AppSettings * self = malloc(sizeof(AppSettings));
    self->queue = queue;
    self->stream = AppSettingsStream__create(priv_AppSettings_state_changed,self);
    AppSettings__load_settings(self);
    AppSettings__create_ui(self);
    AppSettings__reset_settings(self);
    priv_AppSettings_state_changed(self);
    return self;
}

void AppSettings__destroy(AppSettings* self){
    if(self){
        //TODO Clean up more panels here
        AppSettingsStream__destroy(self->stream);
        free(self);
    }
}

void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget){
    self->loading_handle = widget;
}

GtkWidget * AppSettings__get_widget(AppSettings * self){
    return self->widget;
}