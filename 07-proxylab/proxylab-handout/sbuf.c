#include "sbuf.h"

void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->items, 0, 0);
    Sem_init(&sp->slots, 0, n);
}

void subf_destory(sbuf_t *sp){
    Free(sp->buf);
}

void sbuf_add(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[sp->rear] = item;
    sp->rear = (sp->rear +1) % sp->n;
    //printf("Sbuf added, total: %d\n", (sp->rear-sp->front+sp->n)%sp->n);
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp){
    P(&sp->items);
    P(&sp->mutex);
    int item = sp->buf[sp->front];
    sp->front = (sp->front + 1)% sp->n;
    //printf("Sbuf removed, total: %d\n", (sp->rear-sp->front+sp->n)%sp->n);
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}