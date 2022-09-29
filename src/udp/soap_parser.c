#include <string.h>
#include <ctype.h>
#include <libxml/parser.h>

#include "soap_parser.h"


// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

void 
parse_header(xmlNode *pnode, struct DiscoveredServer *server){
    xmlNode        *first_child, *cnode;
    xmlElementType *type;
    char *MSGID_TAG="MessageID";
    char *RELATESTO_TAG="RelatesTo";
    char *tmp;

    first_child = pnode->children;
    for (cnode = first_child; cnode; cnode = cnode->next) {
        if(cnode->type == 1){
            if(strcmp(cnode->name,MSGID_TAG) == 0){
                tmp=trimwhitespace(xmlNodeGetContent(cnode));
                server->msg_uuid = (char *) malloc(strlen(tmp)-4);
                memcpy(server->msg_uuid,&tmp[5],strlen(tmp)-4);
            } else if(strcmp(cnode->name,RELATESTO_TAG) == 0){
                tmp=trimwhitespace(xmlNodeGetContent(cnode));
                server->relate_uuid = (char *) malloc(strlen(tmp)-8);
                memcpy(server->relate_uuid,&tmp[9],strlen(tmp)-8);
            }
        }
    }
}

void 
parse_prob_match(xmlNode *pnode, struct ProbMatch *match){
    xmlNode        *first_child, *cnode;
    xmlElementType *type;
    char *ENDPOINT_REF_TAG="EndpointReference";
    char *TYPES_TAG="Types";
    char *SCOPES_TAG="Scopes";
    char *XADDRS_TAG="XAddrs";
    char *META_VER_TAG="MetadataVersion";

    // match->types = (char *) malloc(0);
    match->types = malloc(0);
    match->prob_uuid = malloc(0);

    first_child = pnode->children;
    for (cnode = first_child; cnode; cnode = cnode->next) {
        char *tmp;
        if(cnode->type == 1){
            if(strcmp(cnode->name,ENDPOINT_REF_TAG) == 0){
                tmp=trimwhitespace(xmlNodeGetContent(cnode));
                match->prob_uuid = realloc(match->prob_uuid, strlen(tmp)-8);
                memcpy(match->prob_uuid,&tmp[9],strlen(tmp)-8);
            } else if(strcmp(cnode->name,TYPES_TAG) == 0){
                tmp=trimwhitespace(xmlNodeGetContent(cnode));
                match->types = realloc(match->types, strlen(tmp)-2);
                memcpy(match->types,&tmp[3],strlen(tmp)-2);
            } else if(strcmp(cnode->name,SCOPES_TAG) == 0){
                char *tmpp=(char *)trimwhitespace(xmlNodeGetContent(cnode));
                int i = 0;
                match->scope_count = 0;
                char *p = strtok (tmpp, "\n");
                char **array = malloc(sizeof (char *) * match->scope_count);

                while (p != NULL)
                {
                    match->scope_count++;
                    tmp=trimwhitespace(p);
                    array = realloc (array,sizeof (char *) * match->scope_count);
                    array[i++] = tmp;
                    p = strtok (NULL, "\n");
                }

                match->scopes=array;
            } else if(strcmp(cnode->name,XADDRS_TAG) == 0){
                match->addr=trimwhitespace(xmlNodeGetContent(cnode));
            } else if(strcmp(cnode->name,META_VER_TAG) == 0){
                match->version=trimwhitespace(xmlNodeGetContent(cnode));
            } else {
                printf("wut : %s\n",cnode->name);
            }
        }
    }
} 

void 
parse_prob_matches(xmlNode *pnode, struct DiscoveredServer *server){
    xmlNode        *first_child, *cnode;
    xmlElementType *type;
    struct ProbMatch matche;
    char *PROBMATCH_TAG="ProbeMatch";

    server->match_count = 0;

    first_child = pnode->children;
    for (cnode = first_child; cnode; cnode = cnode->next) {
        if(cnode->type == 1){

            server->match_count++;
            server->matches = (struct ProbMatch *) realloc (server->matches,sizeof (struct ProbMatch) * server->match_count);
            
            if(strcmp(cnode->name,PROBMATCH_TAG) == 0){
                parse_prob_match(cnode,&server->matches[server->match_count-1]);
            }
        }
    }
}

void 
parse_body(xmlNode *pnode, struct DiscoveredServer *server){
    xmlNode        *first_child, *cnode;
    xmlElementType *type;
    char *PROBMATCHES_TAG="ProbeMatches";

    first_child = pnode->children;
    for (cnode = first_child; cnode; cnode = cnode->next) {
        if(cnode->type == 1){
            if(strcmp(cnode->name,PROBMATCHES_TAG) == 0){
                parse_prob_matches(cnode, server);
            }
        }
    }
}

void 
parse_envelope(xmlNode *pnode, struct DiscoveredServer *server){
    xmlNode        *first_child, *cnode;
    xmlElementType *type;
    char *HEADER_TAG="Header";
    char *BODY_TAG="Body";
    
    first_child = pnode->children;
    for (cnode = first_child; cnode; cnode = cnode->next) {
        if(cnode->type == 1){
            if(strcmp(cnode->name,HEADER_TAG) == 0){
                parse_header(cnode,server);
            } else if(strcmp(cnode->name,BODY_TAG) == 0){
                parse_body(cnode,server);
            }
        }
    }
}

struct DiscoveredServer 
parse_soap_msg (char *msg){

    xmlDoc         *document;
    xmlNode        *root, *first_child, *node;
    struct DiscoveredServer server;
    server.match_count = 0;
    server.matches = (struct ProbMatch *) malloc (sizeof (struct ProbMatch) * server.match_count);

    document = xmlParseDoc(msg);

    root = xmlDocGetRootElement(document);
    parse_envelope(root,&server);

    return server;
}
