#include "clist.h"
#include <stdlib.h>

int priv_CList__remove_element_and_shift(CList* self, CObject **array, int index, int array_length);
void priv_CList__destroy(CObject* self);

CList * CList__create(){
    CList * list = malloc(sizeof(CList));
    CList__init(list);
    CObject__set_destroy_callback((CObject*)list,priv_CList__destroy);
    return list;
}

void CList__init(CList* self){
    CObject__init((CObject*)self);
    self->count = 0;
    self->data = malloc(0);
}

void CList__insert_element(CList* self, CObject * record, int index){
    int i;
    self->data = realloc (self->data,sizeof (record) + sizeof(self->data));
    for(i=self->count; i> index; i--){
        self->data[i] = self->data[i-1];
    }
    self->data[index]=record;
    self->count++;
};

void CList__remove_element(CList* self, int index){
    //Remove element and shift content
    int shrink_size = priv_CList__remove_element_and_shift(self,self->data, index, self->count);  /* First shift the elements, then reallocate */
    
    //Resize count
    self->count--;

    //Resize array memory
    self->data = realloc (self->data,sizeof(self->data)-shrink_size);
}

void CList__clear(CList* self){
    if(self){
        int i;
        for(i=0; i < self->count; i++){
            CObject__destroy(self->data[i]);
        }
        self->count = 0;
        self->data = realloc(self->data,0);
    }
}


int priv_CList__remove_element_and_shift(CList* self, CObject **array, int index, int array_length){
    int i;
    int shrink_size = 0;
    for(i = index; i < array_length; i++) {
        shrink_size = sizeof(array[i]);
        array[i] = array[i + 1];
    }
    return shrink_size;
};

void priv_CList__destroy(CObject * self){
    CList * list = (CList*)self;
    CList__clear(list);
    free(list->data);
}