#include "gui_update_calls.h"
#include "onvif_device.h"
#include "../queue/event_queue.h"
#include "../gst2/player.h"
#include <gtk/gtk.h>
#include <netdb.h>

struct DeviceInput {
  OnvifDevice * device;
  GtkWidget *img_handle;
};

void _display_onvif_thumbnail(void * user_data){
  GtkWidget *image;
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf;

  struct DeviceInput * input = (struct DeviceInput *) user_data;

  //TODO handle access denied...
  // image = gtk_image_new_from_icon_name ("system-lock-screen-symbolic", 1);
  // gtk_container_foreach (GTK_CONTAINER (input->img_handle), (void*) gtk_widget_destroy, NULL);
  // gtk_container_add (GTK_CONTAINER (input->img_handle), image);
  // gtk_widget_show (image);

  struct chunk * imgchunk = OnvifDevice__media_getSnapshot(input->device);

  loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write (loader, imgchunk->buffer, imgchunk->size, NULL);
  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  double ph = gdk_pixbuf_get_height (pixbuf);
  double pw = gdk_pixbuf_get_width (pixbuf);
  double newpw = 40 / ph * pw;
  pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,40,GDK_INTERP_NEAREST);
  image = gtk_image_new_from_pixbuf (pixbuf); 
  gtk_container_foreach (GTK_CONTAINER (input->img_handle), (void*) gtk_widget_destroy, NULL);
  gtk_container_add (GTK_CONTAINER (input->img_handle), image);
  gtk_widget_show (image);

  free(input->device);
  free(input);
}

void display_onvif_thumbnail(OnvifDevice * device, EventQueue * queue, GtkWidget * handle){

  struct DeviceInput * input = malloc(sizeof(struct DeviceInput));
  input->img_handle = handle;
  input->device = OnvifDevice__copy(device);  

  EventQueue__insert(queue,_display_onvif_thumbnail,input);
  
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
}

void display_nslookup_hostname(OnvifDevice * device, EventQueue * queue){
  struct DeviceInput * input = malloc(sizeof(struct DeviceInput));
  input->device = OnvifDevice__copy(device);  

  EventQueue__insert(queue,_display_nslookup_hostname,input);
}