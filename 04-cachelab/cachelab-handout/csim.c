// by gongxuri

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int hits=0,misses=0,evictions=0;
int tick=0;
int *valid,*tag,*used_tick;
int v=0,s,S,E,b,B;
// int tag_pad;


void touch(int s_index, int tag_id){
  //printf(" %d %d", s_index, tag_id);
  int lui = E*s_index;
  for (int i=E*s_index; i<E*(s_index+1); i++){
    if(valid[i] && tag[i] == tag_id){
      printf(" hit");
      hits++;
      used_tick[i] = (tick ++);
      return;
    }
    else{
      if(used_tick[i] < used_tick[lui]){
        lui = i;
      }
    }
  }
  printf(" miss");
  misses++;
  if(valid[lui]){
    printf(" eviction");
    evictions++;
  }
  valid[lui] = 1;
  tag[lui] = tag_id;
  used_tick[lui] = (tick++);
}

void execute(char op, int address, int size){
  int start = address >> b;
  int end = (address + size - 1) >> b;
  int s_index, tag_id;
  for (int i=start; i<=end; i++){
    tag_id = i >> s;
    s_index = i & ((1 << s)-1);
    switch(op){
      case 'L':
        touch(s_index, tag_id);
        break;
      case 'S':
	touch(s_index, tag_id);
        break;
      case 'M':
	touch(s_index, tag_id);
        touch(s_index, tag_id);
	break;
    }
  }
}

int main(int argc, char* argv[])
{
    char* t;
    int ch;

    while((ch=getopt(argc, argv, "vs:E:b:t:"))!=-1){
      switch(ch){
        case 'v':
	  v = 1;
	  break;
	case 's':
          s = atoi(optarg);
	  S = 1<<s;
          break;
	case 'E':
          E = atoi(optarg);
          break;
	case 'b':
          b = atoi(optarg);
	  B = 1<<b;
          break;
	case 't':
	  t = optarg;
	  break;
      }
    }
    printf("v:%d s:%d E:%d b:%d t:%s\n",v,s,E,b,t);
    printf("S:%d E:%d B:%d\n",S,E,B);
    
    // tag_pad = ((1 << s) - 1) << b;
    // printf("tag pad: %X\n", tag_pad);

    valid = (int*) malloc(S*E*sizeof(int));
    tag = (int*) malloc(S*E*sizeof(int));
    used_tick = (int*) malloc(S*E*sizeof(int));
    for (int i =0; i<S*E; i++){
      valid[i] = 0;
      tag[i] = 0;
      used_tick[i] = -1;
    }

    FILE * fp;
    char line[64], op;
    char * token;
    int address, size;
    fp = fopen(t, "r");
    while(fgets(line, sizeof(line), fp)){
      if(line[0] == ' '){
	op = line[1];
	token = strtok(line, " ");
	token = strtok(NULL, ", ");
	//address = atoi(token);
	sscanf(token, "%x", &address);
	token = strtok(NULL, ", ");
	//size = atoi(token);
	sscanf(token, "%x", &size);
	printf("%c %d,%d", op, address, size);
	execute(op, address, size);
	printf("\n");
      }
    }
    free(valid);
    free(tag);
    free(used_tick);
    printSummary(hits, misses, evictions);
    return 0;
}
