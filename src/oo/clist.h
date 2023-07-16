#ifndef C_LIST_H_ 
#define C_LIST_H_

#include <pthread.h>
#include "cobject.h"

typedef struct _CList CList;

struct _CList {
    CObject parent;
    
    int count;
    CObject ** data;
};

CList * CList__create();
void CList__init(CList* self);
void CList__insert_element(CList* self, CObject * record, int index);
void CList__remove_element(CList* self, int index);
void CList__clear(CList* self);

#endif