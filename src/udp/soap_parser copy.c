#include "soap_parser.h"
#include <stdio.h>
#include <string.h>
#include <expat.h>

/* Keep track of the current level in the XML tree */
int             Depth;

#define MAXCHARS 1000000

void
start(void *data, const char *el, const char **attr)
{
    int             i;

    for (i = 0; i < Depth; i++)
        printf("  ");

    printf("x%s", el);

    for (i = 0; attr[i]; i += 2) {
        printf(" #%i# ",i);
        printf(" --%s='%s'", attr[i], attr[i + 1]);
    }
    
    
    printf("\n");
    for (i = 0; attr[i]; i += 2) {
        printf("last %s\n",attr[i]);
    }
    Depth++;
}               /* End of start handler */

void
end(void *data, const char *el)
{
    Depth--;
}               /* End of end handler */


char*
parse_soap_msg (char *msg){

    XML_Parser      parser;


    parser = XML_ParserCreate(NULL);
    if (parser == NULL) {
        fprintf(stderr, "Parser not created\n");
        return "Failed";
    }
    /* Tell expat to use functions start() and end() each times it encounters
     * the start or end of an element. */
    XML_SetElementHandler(parser, start, end);

    if (XML_Parse(parser, msg, strlen(msg), XML_TRUE) ==
        XML_STATUS_ERROR) {
        fprintf(stderr,
            "Cannot parse msg, file may be too large or not well-formed XML\n");
        return "Failed";
    }

    XML_ParserFree(parser);
    fprintf(stdout, "Successfully parsed %i characters in msg\n",strlen(msg));
    
    return "Parsing";
}