#include "gui_update_calls.h"
#include "onvif_device.h"
#include "../queue/event_queue.h"
#include "../gst2/player.h"
#include "../device_list.h"
#include <gtk/gtk.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "credentials_input.h"

extern char _binary_prohibited_icon_png_size[];
extern char _binary_prohibited_icon_png_start[];
extern char _binary_prohibited_icon_png_end[];

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

extern char _binary_warning_png_size[];
extern char _binary_warning_png_start[];
extern char _binary_warning_png_end[];

extern char _binary_microphone_png_size[];
extern char _binary_microphone_png_start[];
extern char _binary_microphone_png_end[];


struct DeviceInput {
  Device * device;
  EventQueue * queue;
};


struct PlayInput {
  OnvifPlayer * player;
  GtkListBoxRow * row;
  EventQueue * queue;
};

struct DeviceInput * DeviceInput_copy(struct DeviceInput * input){
  struct DeviceInput * newinput = malloc(sizeof(struct DeviceInput));
  newinput->device = input->device;//OnvifDevice__copy(input->device);  
  newinput->queue = input->queue;
  return newinput;
}

struct PlayInput * PlayInput_copy(struct PlayInput * input){
  struct PlayInput * newinput = malloc(sizeof(struct PlayInput));
  newinput->player = input->player;//OnvifDevice__copy(input->device);  
  newinput->row = input->row;
  newinput->queue = input->queue;
  return newinput;
}

void _display_onvif_thumbnail(void * user_data, int profile_index){
  GtkWidget *image;
  GdkPixbufLoader *loader = NULL;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled_pixbuf = NULL;
  double size;
  char * imgdata;
  int freeimgdata = 0;

  struct DeviceInput * input = (struct DeviceInput *) user_data;

  if(!Device__is_valid(input->device)){
    goto exit;
  }

  Device__addref(input->device);

  if(input->device->onvif_device->authorized){
    //TODO handle profiles
    struct chunk * imgchunk = OnvifDevice__media_getSnapshot(input->device->onvif_device,profile_index);
    if(!imgchunk){
      //TODO Set error image
      printf("Error retrieve snapshot.");
      goto exit;
    }
    imgdata = imgchunk->buffer;
    size = imgchunk->size;
    freeimgdata = 1;
    free(imgchunk);
  } else {
    imgdata = _binary_locked_icon_png_start;
    size = _binary_locked_icon_png_end - _binary_locked_icon_png_start;
  }

  //If the gtk handle is destoyed by the time its ready
  if(!GTK_IS_CONTAINER(input->device->image_handle)){
    goto exit;
  }

  //Attempt to get downloaded pixbuf or locked icon
  loader = gdk_pixbuf_loader_new ();
  GError *error = NULL;
  if(gdk_pixbuf_loader_write (loader, (unsigned char *)imgdata, size,&error)){
    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  } else {
    if(error->message){
      printf("Error writing png to GtkPixbufLoader : %s\n",error->message);
    } else {
      printf("Error writing png to GtkPixbufLoader : [null]\n");
    }
  }
  
  //Show warning icon in case of failure
  if(!pixbuf){
    if(gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_warning_png_start, _binary_warning_png_end - _binary_warning_png_start,&error)){
      pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    } else {
      if(error->message){
        printf("Error writing warning png to GtkPixbufLoader : %s\n",error->message);
      } else {
        printf("Error writing warning png to GtkPixbufLoader : [null]\n");
      }
    }
  }
  
  //Remove previous image
  gtk_container_foreach (GTK_CONTAINER (input->device->image_handle), (void*) gtk_widget_destroy, NULL);

  //Display pixbuf
  if(pixbuf){
    double ph = gdk_pixbuf_get_height (pixbuf);
    double pw = gdk_pixbuf_get_width (pixbuf);
    double newpw = 40 / ph * pw;
    scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,40,GDK_INTERP_NEAREST);
    image = gtk_image_new_from_pixbuf (scaled_pixbuf);
    gtk_container_add (GTK_CONTAINER (input->device->image_handle), image);
    gtk_widget_show (image);
  } else {
    printf("Failed all thumbnail creation.\n");
  }

exit:
  if(loader){
    gdk_pixbuf_loader_close(loader,NULL);
    g_object_unref(loader);
  }
  if(scaled_pixbuf){
    g_object_unref(scaled_pixbuf);
  }
  if(freeimgdata){
    free(imgdata);
  }
  Device__unref(input->device);
}

void _display_nslookup_hostname(void * user_data){
  struct DeviceInput * input = (struct DeviceInput *) user_data;
  char * hostname;

  if(!Device__is_valid(input->device)){
    goto exit;
  }

  Device__addref(input->device);
  printf("NSLookup ... %s\n",input->device->onvif_device->ip);
  //Lookup hostname
  struct in_addr in_a;
  inet_pton(AF_INET, input->device->onvif_device->ip, &in_a);
  struct hostent* host;
  host = gethostbyaddr( (const void*)&in_a, 
                      sizeof(struct in_addr), 
                      AF_INET );
  if(host){
      printf("Found hostname : %s\n",host->h_name);
      hostname = host->h_name;
  } else {
      printf("Failed to get hostname ...\n");
      hostname = NULL;
  }

  printf("Retrieved hostname : %s\n",hostname);

exit:
  Device__unref(input->device);
  free(input);
}

//WIP Profile selection
void _display_onvif_profiles(void * user_data){
  struct DeviceInput * input = (struct DeviceInput *) user_data;

  if(!input->device->onvif_device->authorized){
    return;
  }

  GtkListStore *liststore = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(input->device->profile_dropdown)));
  OnvifDevice_get_profiles(input->device->onvif_device);
  for (int i = 0; i < input->device->onvif_device->sizeSrofiles; i++){
    printf("Profile name: %s\n", input->device->onvif_device->profiles[i].name);
    printf("Profile token: %s\n", input->device->onvif_device->profiles[i].token);

    // if(i == 0){
    //   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(input->device->profile_dropdown)->entry), input->device->onvif_device->profiles[i].name);
    // }

    // GtkListStore *liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    gtk_list_store_insert_with_values(liststore, NULL, -1,
                                      // 0, "red",
                                      1, input->device->onvif_device->profiles[i].name,
                                      // 2, input->device->onvif_device->profiles[i].token,
                                      -1);
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(input->device->profile_dropdown),0);
  gtk_widget_set_sensitive (input->device->profile_dropdown, TRUE);
  // gtk_combo_set_popdown_strings (GTK_COMBO(input->device->profile_dropdown), cbitems);

}

void _display_onvif_device(void * user_data){
  struct DeviceInput * input = (struct DeviceInput *) user_data;
  /* Start by authenticating the device then start retrieve thumbnail */
  OnvifDevice_authenticate(input->device->onvif_device);

  /* Display Profile dropdown */
  // _display_onvif_profiles(input);

  /* Display row thumbnail. Default to profile index 0 */
  _display_onvif_thumbnail(input, 0);

  free(input);
}

void display_onvif_device_row(Device * device, EventQueue * queue){
  struct DeviceInput * input = malloc(sizeof(struct DeviceInput));
  input->device = device;
  input->queue = queue;
  
  /* nslookup doesn't require onvif authentication. Dispatch event now. */
  EventQueue__insert(queue,_display_nslookup_hostname,DeviceInput_copy(input));

  EventQueue__insert(queue,_display_onvif_device,input);
}

struct PlayStreamInput {
  OnvifPlayer * player;
  char * uri;
};


gboolean * main_thread_dispatch (void * user_data){
  struct PlayInput * input = (struct PlayInput *) user_data;
  CredentialsDialog__show(input->player->dialog,input);
  return FALSE;
}

void _stop_onvif_stream(void * user_data){
  OnvifPlayer * player = (OnvifPlayer *) user_data;
  OnvifPlayer__stop(player);
}

void _play_onvif_stream(void * user_data){
  struct PlayInput * input = (struct PlayInput *) user_data;

  //If the gtk handle is destoyed by the time its ready
  if(!Device__is_valid(input->player->device)){
    goto exit;
  }

  Device__addref(input->player->device);
    /* Set the URI to play */
    //TODO handle profiles
  char * uri = OnvifDevice__media_getStreamUri(input->player->device->onvif_device,0);
  OnvifPlayer__set_playback_url(input->player,uri);
  OnvifPlayer__play(input->player);

exit:
  Device__unref(input->player->device);
  free(input);
}

void _onvif_authentication(void * user_data){
  LoginEvent * event = (LoginEvent *) user_data;
  struct PlayInput * input = (struct PlayInput *) event->user_data;
  OnvifDevice_set_credentials(input->player->device->onvif_device,event->user,event->pass);
  OnvifDevice_authenticate(input->player->device->onvif_device);
  if(!input->player->device->onvif_device->authorized){
    return;
  }

  //Replace locked image with spinner
  GtkWidget * image = gtk_spinner_new ();
  gtk_spinner_start (GTK_SPINNER (image));
  gtk_container_foreach (GTK_CONTAINER (input->player->device->image_handle), (void*) gtk_widget_destroy, NULL);
  gtk_container_add (GTK_CONTAINER (input->player->device->image_handle), image);
  gtk_widget_show (image);

  CredentialsDialog__hide(input->player->dialog);
  display_onvif_device_row(input->player->device,input->queue);
  EventQueue__insert(input->queue,_play_onvif_stream,PlayInput_copy(input));
  free(input);
  free(event);
}


void dialog_cancel_cb(CredentialsDialog * dialog){
  printf("OnvifAuthentication cancelled...\n");
  CredentialsDialog__hide(dialog);
  free(dialog->user_data);
}

void dialog_login_cb(LoginEvent * event){
  printf("OnvifAuthentication attempt...\n");
  struct PlayInput * input = (struct PlayInput *) event->user_data;
  EventQueue__insert(input->queue,_onvif_authentication,LoginEvent_copy(event));
}


void select_onvif_device_row(OnvifPlayer * player,  GtkListBoxRow * row, EventQueue * queue){
  struct PlayInput * input = malloc (sizeof(struct PlayInput));
  input->player = player;
  input->row = row;
  input->queue = queue;

  EventQueue__insert(queue,_stop_onvif_stream,player);

  if(input->row == NULL){
    free(input);
    return;
  }

  //Set newly selected device
  int pos;
  pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (input->row));
  input->player->device = input->player->device_list->devices[pos];

  //Prompt for authentication
  if(!input->player->device->onvif_device->authorized){
    gdk_threads_add_idle((void *)main_thread_dispatch,input);
    return;
  }

  EventQueue__insert(queue,_play_onvif_stream,input);
}

void stop_onvif_stream(OnvifPlayer * player, EventQueue * queue){
  EventQueue__insert(queue,_stop_onvif_stream,player);
}

gboolean
toggle_mic_cb (GtkWidget *widget, gpointer * p, gpointer * p2){
  OnvifPlayer * player = (OnvifPlayer *) p2;
  if(OnvifPlayer__is_mic_mute(player)){
    OnvifPlayer__mic_mute(player,FALSE);
  } else {
    OnvifPlayer__mic_mute(player,TRUE);
  }
  return FALSE;
}

GtkWidget * create_controls_overlay(OnvifPlayer *player){

  GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_microphone_png_start, _binary_microphone_png_end - _binary_microphone_png_start, NULL);

  GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  double ph = gdk_pixbuf_get_height (pixbuf);
  double pw = gdk_pixbuf_get_width (pixbuf);
  double newpw = 30 / ph * pw;
  pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,30,GDK_INTERP_NEAREST);
  GtkWidget * image = gtk_image_new_from_pixbuf (pixbuf); 

  GtkWidget * widget = gtk_button_new ();
  g_signal_connect (widget, "button-press-event", G_CALLBACK (toggle_mic_cb), player);
  g_signal_connect (widget, "button-release-event", G_CALLBACK (toggle_mic_cb), player);

  gtk_button_set_image (GTK_BUTTON (widget), image);

  GtkWidget * fixed = gtk_fixed_new();
  gtk_fixed_put(GTK_FIXED(fixed),widget,10,10); 
  return fixed;
}