#include "clist_ts.h"
#include <stdlib.h>
#include <stdio.h>

void priv_CListTS__remove_element_and_shift(CListTS* self, CObject **array, int index, int array_length);
void priv_CListTS__destroy(CObject* self);
void priv_CListTS__remove(CListTS* self, int index);
CObject * priv_CListTS__get(CListTS * self, int index);

void priv_CListTS__remove_element_and_shift(CListTS* self, CObject **array, int index, int array_length){
    int i;
    for(i = index; i < array_length; i++) {
        array[i] = array[i + 1];
    }
};

void priv_CListTS__destroy(CObject * self){
    CListTS * list = (CListTS*) self;
    if(list->destroy_callback){
        list->destroy_callback(list);
    }
    CListTS__clear(list);
    free(list->data);
}

void priv_CListTS__remove(CListTS* self, int index){
    //Remove element and shift content
    priv_CListTS__remove_element_and_shift(self,self->data, index, self->count);  /* First shift the elements, then reallocate */
    //Update counter
    self->count--;
    //Resize array memory
    self->data = realloc (self->data,sizeof(void *)-self->count);
}

CObject * priv_CListTS__get(CListTS * self, int index){
    return self->data[index];
}

CListTS * CListTS__create(){
    CListTS * list = malloc(sizeof(CListTS));
    CListTS__init(list);
    return list;
}

void CListTS__init(CListTS* self){
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_CListTS__destroy);

    pthread_mutex_init(&self->lock, NULL);

    self->count = 0;
    self->data = malloc(0);
}

void CListTS__add(CListTS* self, CObject * record){
    pthread_mutex_lock(&self->lock);
    self->data = realloc (self->data,sizeof (void *) * (self->count+1));
    self->data[self->count]=record;
    //Trigger event_count inrrement
    self->count++;
    pthread_mutex_unlock(&self->lock);
};

void CListTS__remove(CListTS* self, int index){
    pthread_mutex_lock(&self->lock);
    priv_CListTS__remove(self,index);
    pthread_mutex_unlock(&self->lock);
}

CObject * CListTS__get(CListTS * self, int index){
    CObject * item;
    pthread_mutex_lock(&self->lock);
    item = priv_CListTS__get(self,index);
    pthread_mutex_unlock(&self->lock);
    return item;
}

CObject * CListTS__pop(CListTS * self){
    if(CListTS__get_count(self) == 0){
        return NULL;
    }
    CObject * item;
    pthread_mutex_lock(&self->lock);
    item = priv_CListTS__get(self, 0);
    priv_CListTS__remove(self, 0);
    pthread_mutex_unlock(&self->lock);
    return item;
}

void CListTS__clear(CListTS* self){
    if(self){
        pthread_mutex_lock(&self->lock);
        int i;
        for(i=0; i < self->count; i++){
            CObject__destroy(self->data[i]);
        }
        self->count = 0;
        self->data = realloc(self->data,0);
        pthread_mutex_unlock(&self->lock);
    }
}

int CListTS__get_count(CListTS * self){
    int ret;
    pthread_mutex_lock(&self->lock);
    ret = self->count;
    pthread_mutex_unlock(&self->lock);
    return ret;
}

void CListTS__set_destroy_callback(CListTS* self, void (*destroy_callback)(CListTS *)){
    self->destroy_callback = destroy_callback;
}

void CListTS__remove_record(CListTS * self, CObject * record){
    pthread_mutex_lock(&self->lock);
    int found = -1; 

    int i;
    for(i=0; i < self->count; i++) {
        if(found < 0 && record == self->data[i]){
            found=i;
        }

        if(found > -1){
            if(i == self->count){
                break;
            }
            self->data[i] = self->data[i + 1];
            if(found == i){
                self->count--;
            }
        }
    }

    if(found < 0){
        printf("---------------------\n");
        printf("| ERROR Terminating Thread not found for clean up %i\n", found);
        printf("---------------------\n");
    } else {
        CObject__destroy(record);
    }
    pthread_mutex_unlock(&self->lock);
}