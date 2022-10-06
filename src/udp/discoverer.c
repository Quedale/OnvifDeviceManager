#include "discoverer.h"
#include "soap_parser.h"
#include <pthread.h>
#include <stdio.h>
#include <gtk/gtk.h>

#define PORT     3702 
#define MAXLINE 4096
char * ENVELOP = "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">\n" \
          "              <soap:Header>\n" \
          "                  <wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>\n" \
          "                  <wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Resolve</wsa:Action>\n" \
          "                  <wsa:MessageID>urn:uuid:29206488-217f-4a1d-83c7-da156409aec8</wsa:MessageID>\n" \
          "              </soap:Header>\n" \
          "              <soap:Body>\n" \
          "                  <wsd:Resolve>\n" \
          "                      <wsa:EndpointReference>\n" \
          "                          <wsa:Address>urn:uuid:1c852a4d-b800-1f08-abcd-6cc21717ea2a</wsa:Address>\n" \
          "                      </wsa:EndpointReference>\n" \
          "                  </wsd:Resolve>\n" \
          "              </soap:Body>\n" \
          "          </soap:Envelope>";



struct DiscoverThreadInput {
    struct UdpDiscoverer * server;
    void * widget;
    void * data;
    void * callback;
    void * done_callback;
};


void UdpDiscoverer__init(struct UdpDiscoverer* self, void * func, void * done_func) {

    self->done_callback = (void *) malloc(sizeof(void));
    self->done_callback = done_func;

    self->found_callback =  (void *) malloc(sizeof(void));
    self->found_callback = func;

}

struct UdpDiscoverer UdpDiscoverer__create(void * func, void * done_func) {
    struct UdpDiscoverer result;
    memset (&result, 0, sizeof (result));
    UdpDiscoverer__init(&result,func, done_func);
    return result;
}


void UdpDiscoverer__reset(struct UdpDiscoverer* self) {
}

void UdpDiscoverer__destroy(struct UdpDiscoverer* self) {
  if (self) {
     UdpDiscoverer__reset(self);
     free(self);
  }
}

void *scan(void * vargp) {
    struct sockaddr_in     servaddr; 
    int sockfd; 
    char buffer[MAXLINE]; 
    DiscoveredServer server;
    DiscoveryEvent * ret_event; 
    int n, len; //Used by the socket

    //Initialize memory
    ret_event = malloc(0);
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&server, 0, sizeof(server)); 
    
    //Cast to expected struct
    struct DiscoverThreadInput * in = (struct DiscoverThreadInput *) vargp;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    //Set broacast setting
    int broadcastEnable=1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Error");
    }

    //Set timeout setting
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 100000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
        
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    
    //Send envelop udp request
    sendto(sockfd, (const char *)ENVELOP, strlen(ENVELOP), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 

    //Looping until timeout
    do {

        //Reallocate event memory
        ret_event =  (DiscoveryEvent *) realloc(ret_event,sizeof(DiscoveryEvent)); 
        ret_event->player = in->data;

        //Reset buffer
        memset(buffer, 0, sizeof(buffer));

        //Reading response
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 

        if (n > 0){
            //Wrap buffer
            buffer[n] = '\0'; 
            //Extract struct and set pointer on event struct
            server = parse_soap_msg(buffer);
            ret_event->server = &server;
            ret_event->widget = in->widget;

            //Dispatch notification about found server
            gdk_threads_add_idle(in->callback,ret_event);
        }

    //Loop until timeout
    } while (n > 0);

    close(sockfd); 

    //Dispatch notification of completion
    gdk_threads_add_idle(in->done_callback,ret_event);
}

void * UdpDiscoverer__start(struct UdpDiscoverer* self, void * widget, void *player) {

    pthread_t thread_id;

    struct DiscoverThreadInput * in = (struct DiscoverThreadInput *) malloc(sizeof(struct DiscoverThreadInput));
    in->server = self;
    in->data = player;
    in->callback = self->found_callback;
    in->done_callback = self->done_callback;
    in->widget=widget;

    pthread_create(&thread_id, NULL, scan, (void *)in);
}