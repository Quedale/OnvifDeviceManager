#include "discovery.h"
#include "soap_parser.h"

#define PORT     3702 
#define MAXLINE 4096 

struct DiscoveredServer udp_discover() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *envelop; 
    struct sockaddr_in     servaddr; 
    struct DiscoveredServer server;

    //TODO Use generated uuid
    envelop="<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">\n" \
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
          
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 100000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    memset(&servaddr, 0, sizeof(servaddr)); 
        
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
        
    int n, len; 
        
    sendto(sockfd, (const char *)envelop, strlen(envelop), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    printf("Envelop message sent.\n"); 

    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len); 

    if (strlen(buffer) > 6){
        buffer[n] = '\0'; 
        server = parse_soap_msg(buffer);
    } else {
        server.msg_uuid = "";
        server.relate_uuid = "";
        server.match_count = 0;
    }

    close(sockfd); 
    return server; 
}