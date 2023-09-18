#include "cobject.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>


void CObject__set_destroyed(CObject * self){
    if(self){
        pthread_mutex_lock(self->ref_lock);
        self->destroyed = 1;
        pthread_mutex_unlock(self->ref_lock);

    }
}

void CObject__set_destroy_callback(CObject* self, void (*destroy_callback)(CObject *)){
    self->destroy_callback = destroy_callback;
}

void CObject__init(CObject* self) {
    self->refcount = 1;
    self->destroyed = 0;
    self->allocated = 0;
    self->destroy_callback = NULL;
    self->ref_lock =malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(self->ref_lock, NULL);
}

CObject * CObject__create(){
    CObject* result = (CObject*) malloc(sizeof(CObject));
    CObject__init(result);
    result->allocated = 1;
    return result;
}

void CObject__destroy(CObject* self) {
    if (self && CObject__ref_count(self) == 0) {
        if(self->destroy_callback){
            self->destroy_callback(self);
        }
        pthread_mutex_destroy(self->ref_lock);
        free(self->ref_lock);
        if(self->allocated){
            self->allocated = 0;
            free(self);
        }
    } else if(self){
        CObject__set_destroyed(self);
        CObject__unref((CObject*)self);
    }
}

int CObject__addref(CObject *self){
    if(CObject__is_valid((CObject*)self)){
        pthread_mutex_lock(self->ref_lock);
        self->refcount++;
        pthread_mutex_unlock(self->ref_lock);
        return 1;
    }

    return 0;
}

void CObject__unref(CObject *self){
    if(self){
        pthread_mutex_lock(self->ref_lock);
        self->refcount--;
        if(self->refcount == 0){
            pthread_mutex_unlock(self->ref_lock);
            CObject__destroy(self);
        } else {
            pthread_mutex_unlock(self->ref_lock);
        }
    }
}

int CObject__ref_count(CObject *self){
    int ret = -1;
    if(self){
        pthread_mutex_lock(self->ref_lock);
        ret = self->refcount;
        pthread_mutex_unlock(self->ref_lock);
    }
    return ret;
}


int CObject__is_destroyed(CObject * self){
    int ret = -1;
    if(self){
        pthread_mutex_lock(self->ref_lock);
        ret = self->destroyed;
        pthread_mutex_unlock(self->ref_lock);

    }
    return ret;
}

int CObject__is_valid(CObject* self){
    if(self && CObject__ref_count(self) > 0 && !CObject__is_destroyed(self)){
        return 1;
    }

    return 0;
}