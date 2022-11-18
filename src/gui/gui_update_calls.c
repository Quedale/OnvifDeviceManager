#include "gui_update_calls.h"
#include "onvif_device.h"
#include "../queue/event_queue.h"
#include "../gst2/player.h"
#include <gtk/gtk.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

extern char _binary_prohibited_icon_png_size[];
extern char _binary_prohibited_icon_png_start[];
extern char _binary_prohibited_icon_png_end[];

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

struct DeviceInput {
  OnvifDevice * device;
  EventQueue * queue;
  GtkWidget *thumbnail_handle;
};


struct DeviceInput * copy_input(struct DeviceInput * input){
  struct DeviceInput * newinput = malloc(sizeof(struct DeviceInput));
  newinput->thumbnail_handle = input->thumbnail_handle;
  newinput->device = OnvifDevice__copy(input->device);  
  newinput->queue = input->queue;
  return newinput;
}

void _display_onvif_thumbnail(void * user_data){
  GtkWidget *image;
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf;

  struct DeviceInput * input = (struct DeviceInput *) user_data;
  double size;
  char * imgdata;

  if(input->device->authorized){
    struct chunk * imgchunk = OnvifDevice__media_getSnapshot(input->device);
    imgdata = imgchunk->buffer;
    size = imgchunk->size;
    free(imgchunk);
  } else {
    imgdata = _binary_locked_icon_png_start;
    size = _binary_locked_icon_png_end - _binary_locked_icon_png_start;
  }

  loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (loader, imgdata, size, NULL);
  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  double ph = gdk_pixbuf_get_height (pixbuf);
  double pw = gdk_pixbuf_get_width (pixbuf);
  double newpw = 40 / ph * pw;
  pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,40,GDK_INTERP_NEAREST);
  image = gtk_image_new_from_pixbuf (pixbuf); 
  gtk_container_foreach (GTK_CONTAINER (input->thumbnail_handle), (void*) gtk_widget_destroy, NULL);
  gtk_container_add (GTK_CONTAINER (input->thumbnail_handle), image);
  gtk_widget_show (image);
  
  free(input);
}

void _display_nslookup_hostname(void * user_data){
  struct DeviceInput * input = (struct DeviceInput *) user_data;
  char * hostname; // = OnvifDevice__device_getHostname(input->device);

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

  /* nslookup doesn't require onvif authentication. Dispatch event now. */
  EventQueue__insert(input->queue,_display_nslookup_hostname,copy_input(input));

  /* Start by authenticating the device then start retrieve thumbnail */
  OnvifDevice_authenticate(input->device);

  /* Display row thumbnail */
  EventQueue__insert(input->queue,_display_onvif_thumbnail,copy_input(input));

  free(input);
}

void display_onvif_device_row(OnvifDevice * device, EventQueue * queue, GtkWidget * thumbnail_handle){
  struct DeviceInput * input = malloc(sizeof(struct DeviceInput));
  input->thumbnail_handle = thumbnail_handle;
  input->device = device;
  input->queue = queue;

  EventQueue__insert(queue,_display_onvif_device,input);
}

struct PlayStreamInput {
  OnvifPlayer * player;
  char * uri;
};

struct PlayInput {
  OnvifPlayer * player;
  GtkListBoxRow * row;
};

void _play_onvif_stream(void * user_data){
  struct PlayInput * input = (struct PlayInput *) user_data;
  // OnvifPlayer * player = (OnvifPlayer *) user_data;
  OnvifPlayer__stop(input->player);
  //Unselected. Stopping stream TODO this isn't enough
  if(input->row == NULL){
    return;
  }
  int pos;
  pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (input->row));
  
  input->player->device = input->player->onvifDeviceList->devices[pos];
  if(!input->player->device->authorized){
    return;
  }

    /* Set the URI to play */
  char * uri = OnvifDevice__media_getStreamUri(input->player->device);
  OnvifPlayer__set_playback_url(input->player,uri);
  OnvifPlayer__play(input->player);

}

void select_onvif_device_row(OnvifPlayer * player,  GtkListBoxRow * row, EventQueue * queue){
  printf("select_onvif_device_row %p\n",(void *)player);
  struct PlayInput * input = malloc (sizeof(struct PlayInput));
  input->player = player;
  input->row = row;
  EventQueue__insert(queue,_play_onvif_stream,input);
}

void _stop_onvif_stream(void * user_data){
  OnvifPlayer * player = (OnvifPlayer *) user_data;
  OnvifPlayer__stop(player);
}

void stop_onvif_stream(OnvifPlayer * player, EventQueue * queue){
  EventQueue__insert(queue,_stop_onvif_stream,player);
}