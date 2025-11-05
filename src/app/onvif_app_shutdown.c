#include "onvif_app_shutdown.h"
#include "clogger.h"

gboolean safely_quit_gtk_main(GApplication * application){
    g_application_quit(application);
    return FALSE;
}
void * _thread_destruction(void * event){
    c_log_set_thread_color(ANSI_COLOR_RED, P_THREAD_ID);
    C_INFO("Starting clean up thread...\n");
    OnvifApp * app = ONVIFMGR_APP(event);

    //This what may take a long time.
    //Destroying the EventQueue will hang until all threads are finished
    OnvifApp__destroy(app);
    
    //Quitting from idle thread allows the windows and OnvifMgrDeviceRow (and nested OnvifDevice) to destroy properly
    //Using LOW priority to allow other event to run first.
    g_idle_add_full (G_PRIORITY_LOW, G_SOURCE_FUNC(safely_quit_gtk_main), app, NULL);

    pthread_exit(0);
}

void force_shutdown_cb(OnvifMgrMsgDialog * dialog){
    C_WARN("Aborting GTK main thread!!");
    gtk_main_quit();
}

int ONVIF_APP_SHUTTING_DOWN = 0;

void onvif_app_shutdown(OnvifApp * data){
    if(!ONVIF_APP_SHUTTING_DOWN){
        C_INFO("Calling App Shutdown...");
        ONVIF_APP_SHUTTING_DOWN = 1;
    } else {
        C_WARN("Application already shutting down...");
        return;
    }
    OnvifMgrMsgDialog * dialog = OnvifMgrMsgDialog__new();
    g_signal_connect (G_OBJECT (dialog), "submit", G_CALLBACK (force_shutdown_cb), NULL);
    g_object_set (dialog, "closable", FALSE, NULL);
    g_object_set (dialog, "cancellable", FALSE, NULL);
    g_object_set (dialog, "submit-label", "Force Shutdown!", NULL);
    g_object_set (dialog, "title-label", "Shutting down...", NULL);
    OnvifApp__show_msg_dialog(data,dialog);
    OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(dialog),"Waiting for running task to finish...");
    g_object_set (dialog, "action-visible", TRUE, NULL); //Setting this attribute after loading, because it hies the actionpanel
    pthread_t pthread;
    pthread_create(&pthread, NULL, _thread_destruction, data);
    pthread_detach(pthread);
}