#include "onvif_app_shutdown.h"
#include "../animations/dotted_slider.h"
#include "../gst/player.h"

gboolean * gui_quit_main (void * user_data){
    printf("Stopping main thread...\n");
    gtk_main_quit();
    return FALSE;
}

void * _thread_destruction(void * event){
    printf("Starting clean up thread...\n");
    OnvifApp * app = (OnvifApp *) event;

    RtspPlayer__stop(OnvifApp__get_player(app));

    //This what may take a long time.
    //Destroying the EventQueue will hang until all threads are finished
    OnvifApp__destroy(app);

    gdk_threads_add_idle((void *)gui_quit_main,app);

    pthread_exit(0);
}

gboolean * gui_destruction (void * user_data){
    OnvifApp *data = (OnvifApp *) user_data;
    
    GtkWidget * image = create_dotted_slider_animation(10,1);
    MsgDialog * dialog = OnvifApp__get_msg_dialog(data);
    MsgDialog__set_icon(dialog, image);
    AppDialog__set_closable((AppDialog*)dialog, 0);
    AppDialog__hide_actions((AppDialog*)dialog);
    AppDialog__set_title((AppDialog*)dialog,"Shutting down...");
    MsgDialog__set_message(dialog,"Waiting for running task to finish...");
    AppDialog__show((AppDialog *) dialog,NULL,NULL,data);

    pthread_t pthread;
    pthread_create(&pthread, NULL, _thread_destruction, data);

    return FALSE;
}


void onvif_app_shutdown(OnvifApp * self){
    printf("Calling App Shutdown...\n");
    gdk_threads_add_idle((void *)gui_destruction,self);
}