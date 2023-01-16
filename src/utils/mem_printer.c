#include <stdio.h>
#include "mem_printer.h"

void read_off_memory_status(statm_t * result)
{
  unsigned long dummy;
  const char* statm_path = "/proc/self/statm";

  FILE *f = fopen(statm_path,"r");
  if(!f){
    perror(statm_path);
    abort();
  }
  if(7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",
    &result->size,&result->resident,&result->share,&result->text,&result->lib,&result->data,&result->dt))
  {
    perror(statm_path);
    abort();
  }
  fclose(f);
}

void printmemusage(){
  statm_t stat;
  memset(&stat, 0, sizeof(stat));
  read_off_memory_status(&stat);
  //,resident,share,text,lib,data,dt;
  printf("size %ld\n",stat.size);
  printf("resident %ld\n",stat.resident);
  printf("share %ld\n",stat.share);
  printf("text %ld\n",stat.text);
  printf("lib %ld\n",stat.lib);
  printf("data %ld\n",stat.data);
  printf("dt %ld\n",stat.dt);

}