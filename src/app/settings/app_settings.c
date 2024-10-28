#include "app_settings.h"
#include "app_settings_discovery.h"
#include "clogger.h"
#include <pwd.h>
#include <errno.h>

#define CONFIG_FILE_PATH "onvifmgr_settings.ini"


void set_settings_state(AppSettings * settings, int state){
    AppSettingsStream__set_state(settings->stream, state);
    AppSettingsDiscovery__set_state(settings->discovery, state);
    //More settings to add here
}

void set_button_state(AppSettings * settings, int state){
    if(GTK_IS_WIDGET(settings->notice))
        gtk_widget_set_visible(settings->notice,state);
    if(GTK_IS_WIDGET(settings->apply_button))
        gtk_widget_set_sensitive(settings->apply_button,state);
    if(GTK_IS_WIDGET(settings->reset_btn))
        gtk_widget_set_sensitive(settings->reset_btn,state);
}

//Invoked by discovery (TODO Removed for gobject callback)
void priv_AppSettings_state_changed(void * data){
    AppSettings * self = (AppSettings*) data;
    //More settings to add here
    if(AppSettingsStream__get_state(self->stream) ||
        AppSettingsDiscovery__get_state(self->discovery)){
        set_button_state(self,TRUE);
    } else {
        set_button_state(self,FALSE);
    }
}

void priv_AppSettings_state_changed_cb(GtkWidget * widget, AppSettings * self){
    if(AppSettingsStream__get_state(self->stream) ||
        AppSettingsDiscovery__get_state(self->discovery)){
        set_button_state(self,TRUE);
    } else {
        set_button_state(self,FALSE);
    }
}

//GUI callback when apply finished
gboolean save_done(void * user_data){
    AppSettings * settings = (AppSettings *) user_data;
    priv_AppSettings_state_changed(settings);
    set_settings_state(settings,TRUE);
    gtk_spinner_stop (GTK_SPINNER (settings->loading_handle));
    return FALSE;
}

char * AppSettings__get_config_path(){

    char * ret;
    const char * configdir;
    if((configdir = getenv("XDG_CONFIG_HOME")) != NULL){
        ret = malloc(strlen(configdir)+strlen(CONFIG_FILE_PATH)+2);
        strcpy(ret,configdir);
        strcat(ret,"/");
        strcat(ret,CONFIG_FILE_PATH);

        C_TRACE("Using XDG_CONFIG_HOME config directory : %s\n",configdir);
        return ret;
    }

    const char *homedir;
    char *buf = NULL;
    if ((homedir = getenv("HOME")) == NULL) {
        struct passwd pwd;
        struct passwd *result;
        size_t bufsize;
        int s;
        bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize == (unsigned int) -1)
            bufsize = 0x4000; // = all zeroes with the 14th bit set (1 << 14)
        buf = malloc(bufsize);
        if (buf == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        s = getpwuid_r(getuid(), &pwd, buf, bufsize, &result);
        if (result == NULL) {
            if (s == 0)
                printf("Not found\n");
            else {
                errno = s;
                perror("getpwnam_r");
            }
            exit(EXIT_FAILURE);
        }
        homedir = result->pw_dir;
    }

    C_TRACE("Generating default config path from HOME directory : %s\n",homedir);

    ret = malloc(strlen(homedir)+strlen("/.config/") + strlen(CONFIG_FILE_PATH)+1);
    strcpy(ret,homedir);
    strcat(ret,"/.config/");
    strcat(ret,CONFIG_FILE_PATH);

    if(buf) free(buf);

    return ret;
}

//Background task to save settings (invokes internal callbacks)
void _save_settings(QueueEvent * qevt, void * user_data){
    AppSettings * self = (AppSettings *) user_data;

    char * path = AppSettings__get_config_path();

    C_INFO("Save to setting file : '%s'",path);

    FILE * fptr = fopen(path,"w");

    if(fptr != NULL){
        fprintf(fptr,"%s\n\n",AppSettingsStream__save(self->stream));
        fprintf(fptr,"%s\n\n",AppSettingsDiscovery__save(self->discovery));
        //More settings to add here
        
        fclose(fptr);
    } else {
        C_ERROR("Failed to write to settings file!\n");
    }

    gdk_threads_add_idle(G_SOURCE_FUNC(save_done),self);
    free(path);
}

// Apply button GUI callback
void apply_settings (GtkWidget *widget, AppSettings * settings) {
    set_settings_state(settings,FALSE);
    set_button_state(settings,FALSE);
    gtk_spinner_start (GTK_SPINNER (settings->loading_handle));

    OnvifApp__dispatch(settings->app,settings->app,_save_settings,settings,NULL);
}

void AppSettings__reset_settings(AppSettings * self){
    AppSettingsStream__reset(self->stream);
    AppSettingsDiscovery__reset(self->discovery);
    //More settings to add here
}

void reset_settings (GtkWidget *widget, AppSettings * settings) {
    AppSettings__reset_settings(settings);
    //More settings to add here
}

void add_panel (GtkWidget * notebook, char * title,  GtkWidget * widget){
    GtkWidget * label, * scroll;

    //Add stream page to notebook
    label = gtk_label_new (title);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (scroll, TRUE);
    gtk_widget_set_vexpand (scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll),widget);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scroll, label);
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

    add_panel(notebook, "Discovery", AppSettingsDiscovery__get_widget(self->discovery));
    add_panel(notebook, "Stream", GTK_WIDGET(self->stream));

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
    
    AppSettingsType category = APPSETTING_INVALID;
    
    char * path = AppSettings__get_config_path();

    C_INFO("Reading Settings file %s",path);
    if (access(path, F_OK) == 0) {
        fptr = fopen(path,"r");
        if(fptr != NULL){
            char buffer[MAX_LEN];
            while (fgets(buffer, MAX_LEN, fptr)){
                // Remove trailing newline
                buffer[strcspn(buffer, "\n")] = 0;

                if(buffer[0] == '['){
                    int newlen = strlen(buffer)-2;
                    char cat[newlen];
                    strncpy(cat,buffer + 1,newlen);
                    cat[newlen] = '\0'; //Mark end of string

                    if(strcmp(AppSettingsStream__get_category(self->stream),cat) == 0){
                        category = APPSETTING_STREAM_TYPE;
                        C_INFO("[%s]",AppSettingsStream__get_category(self->stream));
                    } else if(strcmp(AppSettingsDiscovery__get_category(self->discovery),cat) == 0){
                        category = APPSETTING_DISCOVERY_TYPE;
                        C_INFO("[%s]",AppSettingsDiscovery__get_category(self->discovery));
                    }//More settings to add here

                    continue;
                } else if(category != APPSETTING_INVALID){
                    char * buff_ptr = (char*)buffer;
                    char * key = strtok_r (buff_ptr, "=", &buff_ptr);
                    if(key == NULL){ //Handles empty lines
                        continue;
                    }
                    char * val = strtok_r (buff_ptr, "\n", &buff_ptr); //Get reminder of line
                    C_INFO("\t Property : '%s' = '%s'",key,val);

                    GParamSpec * spec;
                    switch(category){
                        case APPSETTING_STREAM_TYPE:
                            spec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->stream),key);
                            if(spec){
                                GParamSpecClass * klass = G_PARAM_SPEC_GET_CLASS(spec);
                                if(klass->value_type == G_TYPE_INT){
                                    int i;
                                    if(sscanf(val, "%d", &i)){
                                        g_object_set (self->stream, key, i, NULL);
                                    } else {
                                        C_ERROR("Invalid integer value for stream property %s=%s",key,val);
                                    }
                                } else if(klass->value_type == G_TYPE_STRING){
                                    g_object_set (self->stream, key, val, NULL);
                                } else {
                                    C_ERROR("Unsupported GParamSpec value type.");
                                }
                            } else {
                                C_ERROR("Unknown stream property %s=%s",key,val);
                            }
                            break;
                        case APPSETTING_DISCOVERY_TYPE:
                            if(!AppSettingsDiscovery__set_property(self->discovery,key,val)){
                                C_ERROR("Unknown discovery property %s=%s",key,val);
                            }//More settings to add here
                            break;
                        default:
                            //TODO Warning
                            break;
                    }
                    continue;
                } else {
                    C_WARN("Setting without category");
                }
            }
        } else {
            C_WARN("No config file found. Using default configs. 2");
        }
    } else {
        C_WARN("No config file found. Using default configs. 1");
    }

    free(path);
    if(fptr)
        fclose(fptr);
}

AppSettings * AppSettings__create(OnvifApp * app){
    AppSettings * self = malloc(sizeof(AppSettings));
    self->app = app;
    self->stream = AppSettingsStream__new();
    g_signal_connect(self->stream,"settings-changed",G_CALLBACK(priv_AppSettings_state_changed_cb),self);
    self->discovery = AppSettingsDiscovery__create(priv_AppSettings_state_changed,self);
    AppSettings__load_settings(self);
    AppSettings__create_ui(self);
    AppSettings__reset_settings(self);
    priv_AppSettings_state_changed(self);
    return self;
}

void AppSettings__destroy(AppSettings* self){
    if(self){
        AppSettingsDiscovery__destroy(self->discovery);
        free(self);
    }
}

void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget){
    self->loading_handle = widget;
}

GtkWidget * AppSettings__get_widget(AppSettings * self){
    return self->widget;
}