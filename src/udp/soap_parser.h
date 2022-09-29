
#ifndef SOAP_PARSER_H_   /* Include guard */
#define SOAP_PARSER_H_


struct ProbMatch {
    char *prob_uuid;
    char *addr_uuid; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/d:ProbeMatch
    char *addr; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/d:ProbeMatch/wsa:EndpointReference/wsa:Address
    char *types; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/d:ProbeMatch/d:Types
    int scope_count;
    char **scopes; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/d:ProbeMatch/d:Scopes
    char *service; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/d:ProbeMatch/d:XAddrs
    char *version; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/d:ProbeMatch/d:MetadataVersion
};

#define MAX_MATCHES 100

struct DiscoveredServer {
    char *msg_uuid; // SOAP-ENV:Envelope/SOAP-ENV:Header/wsa:MessageID
    char *relate_uuid; // SOAP-ENV:Envelope/SOAP-ENV:Header/wsa:RelatesTo
    struct ProbMatch *matches; // SOAP-ENV:Envelope/SOAP-ENV:Body/d:ProbeMatches/
    int match_count;
};


struct DiscoveredServer
parse_soap_msg (char *msg);

#endif