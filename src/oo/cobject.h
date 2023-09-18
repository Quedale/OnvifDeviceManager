#ifndef C_OBJECT_H_ 
#define C_OBJECT_H_

#include <pthread.h>
typedef struct _CObject CObject;

struct _CObject {
    int destroyed;
    int refcount;
    int allocated;
    pthread_mutex_t * ref_lock;
    void (*destroy_callback)(CObject *);
};

void CObject__destroy(CObject* self);
CObject * CObject__create();
void CObject__init(CObject* self);
int CObject__addref(CObject* device); 
void CObject__unref(CObject* device); 
int CObject__is_valid(CObject* device);
int CObject__ref_count(CObject *self);
void CObject__set_destroy_callback(CObject* self, void (*destroy_callback)(CObject *)); 

#endif