
#ifndef UDP_DISCOVERER_H_   /* Include guard */
#define UDP_DISCOVERER_H_

#include <glib.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include "soap_parser.h"

struct invoke_context
{
  GThreadFunc func;
  gpointer data;
  GMutex lock;
  GCond cond;
  gboolean fired;

  gpointer res;
};

struct UdpDiscoverer {
    struct sockaddr_in     *servaddr; 
    void * found_callback;
    void * done_callback;
};


typedef struct {
    DiscoveredServer * server;
    void * widget;
    void * player;

} DiscoveryEvent;


struct UdpDiscoverer UdpDiscoverer__create(void * func, void * done_func); 
void UdpDiscoverer__destroy(struct UdpDiscoverer* self); 
void * UdpDiscoverer__start(struct UdpDiscoverer* self, void * widget, void * player);

#endif