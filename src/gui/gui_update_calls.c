#include "gui_update_calls.h"
#include "onvif_device.h"
#include "../queue/event_queue.h"
#include "../gst2/player.h"
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

extern char _binary_microphone_png_size[];
extern char _binary_microphone_png_start[];
extern char _binary_microphone_png_end[];


struct DeviceInput {
  OnvifDevice * device;
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

void _display_onvif_thumbnail(void * user_data){
  GtkWidget *image;
  GdkPixbufLoader *loader = NULL;
  GdkPixbuf *pixbuf;
  GdkPixbuf *scaled_pixbuf = NULL;

  struct DeviceInput * input = (struct DeviceInput *) user_data;
  double size;
  char * imgdata;
  int freeimgdata = 0;
  if(input->device->authorized){
    struct chunk * imgchunk = OnvifDevice__media_getSnapshot(input->device);
    if(!imgchunk){
      //TODO Set error image
      printf("Error retrieve snapshot.");
      return;
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
  loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (loader, (unsigned char *)imgdata, size, NULL);
  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  double ph = gdk_pixbuf_get_height (pixbuf);
  double pw = gdk_pixbuf_get_width (pixbuf);
  double newpw = 40 / ph * pw;
  scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,40,GDK_INTERP_NEAREST);
  image = gtk_image_new_from_pixbuf (scaled_pixbuf); 
  gtk_container_foreach (GTK_CONTAINER (input->device->image_handle), (void*) gtk_widget_destroy, NULL);
  gtk_container_add (GTK_CONTAINER (input->device->image_handle), image);
  gtk_widget_show (image);
  
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
}

void _display_nslookup_hostname(void * user_data){
  struct DeviceInput * input = (struct DeviceInput *) user_data;
  char * hostname;

  printf("NSLookup ... %s\n",input->device->ip);
  //Lookup hostname
  struct in_addr in_a;
  inet_pton(AF_INET, input->device->ip, &in_a);
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

  free(input);
}

void _display_onvif_device(void * user_data){
  struct DeviceInput * input = (struct DeviceInput *) user_data;
  /* Start by authenticating the device then start retrieve thumbnail */
  OnvifDevice_authenticate(input->device);

  /* Display row thumbnail */
  _display_onvif_thumbnail(input);

  free(input);
}

void display_onvif_device_row(OnvifDevice * device, EventQueue * queue){
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

void _play_onvif_stream(void * user_data){
  struct PlayInput * input = (struct PlayInput *) user_data;
  // OnvifPlayer * player = (OnvifPlayer *) user_data;
  OnvifPlayer__stop(input->player);
  //Unselected. Stopping stream TODO this isn't enough
  if(input->row == NULL){
    free(input);
    return;
  }
  int pos;
  pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (input->row));
  
  input->player->device = input->player->onvifDeviceList->devices[pos];
  if(!input->player->device->authorized){
    gdk_threads_add_idle((void *)main_thread_dispatch,input);
    return;
  }

    /* Set the URI to play */
  char * uri = OnvifDevice__media_getStreamUri(input->player->device);
  OnvifPlayer__set_playback_url(input->player,uri);
  OnvifPlayer__play(input->player);

  free(input);
}

void _onvif_authentication(void * user_data){
  LoginEvent * event = (LoginEvent *) user_data;
  struct PlayInput * input = (struct PlayInput *) event->user_data;
  OnvifDevice_set_credentials(input->player->device,event->user,event->pass);
  OnvifDevice_authenticate(input->player->device);
  if(!input->player->device->authorized){
    return;
  }
  CredentialsDialog__hide(input->player->dialog);
  EventQueue__insert(input->queue,_play_onvif_stream,PlayInput_copy(input));
  display_onvif_device_row(input->player->device,input->queue);
  free(input);
  free(event);
}


void dialog_cancel_cb(CredentialsDialog * dialog){
  CredentialsDialog__hide(dialog);
  free(dialog->user_data);
}

void dialog_login_cb(LoginEvent * event){
  struct PlayInput * input = (struct PlayInput *) event->user_data;
  EventQueue__insert(input->queue,_onvif_authentication,LoginEvent_copy(event));
}


void select_onvif_device_row(OnvifPlayer * player,  GtkListBoxRow * row, EventQueue * queue){
  struct PlayInput * input = malloc (sizeof(struct PlayInput));
  input->player = player;
  input->row = row;
  input->queue = queue;
  
  EventQueue__insert(queue,_play_onvif_stream,input);
}

void _stop_onvif_stream(void * user_data){
  OnvifPlayer * player = (OnvifPlayer *) user_data;
  OnvifPlayer__stop(player);
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