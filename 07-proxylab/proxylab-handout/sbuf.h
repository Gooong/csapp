#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"

typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void subf_destory(sbuf_t *sp);
void sbuf_add(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

#endif