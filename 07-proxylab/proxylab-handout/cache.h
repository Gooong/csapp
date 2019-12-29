#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
    char **fingers;
    unsigned long *timestamps;
    int *valids;
    char **objects;
    size_t *obj_sizes;
    int num_obj;
    int global_time;
    int read_count;
    sem_t mutex, readable, writable;
} cache_t;

void cache_init(cache_t *cp, int n);

void cache_destory(cache_t *cp);

int get_obj(cache_t *cp, char *finger, char *dest, size_t *lengthp);

void store_obj(cache_t *cp, char *finger, char *content, size_t lenght);

#endif