#ifndef MEM_PRINTER_H_ 
#define MEM_PRINTER_H_

typedef struct {
    unsigned long size,resident,share,text,lib,data,dt;
} statm_t;


void printmemusage();

#endif