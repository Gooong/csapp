#include "cache.h"

void cache_init(cache_t *cp, int n){
    cp->fingers = (char **)Malloc(n*sizeof(char*));
    cp->objects = (char **)Malloc(n*sizeof(char*));
    cp->timestamps = (unsigned long *)Calloc(n, sizeof(unsigned long));
    cp->valids = (int *)Calloc(n, sizeof(int));
    cp->obj_sizes = (size_t *)Calloc(n, sizeof(size_t));
    for(int i = 0; i < n; i++){
        cp->fingers[i] = (char *)Malloc(MAXLINE);
        cp->objects[i] = (char *)Malloc(MAX_OBJECT_SIZE);
    }
    cp->global_time = 1;
    cp->num_obj = n;
    cp->read_count = 0;
    Sem_init(&cp->mutex, 0, 1);
    Sem_init(&cp->writable, 0, 1);
    Sem_init(&cp->readable, 0, 1);
}

int get_obj(cache_t *cp, char *finger, char *dest, size_t *lengthp){
    // printf("Searching: %s\n", finger);
    // printf("Showing cache:\n");
    // for(int i = 0; i<cp->num_obj; i++){
    //     if (cp->valids[i]){
    //         printf("%d: size:%ld cmp:%d %s\n", i, cp->obj_sizes[i], strcmp(cp->fingers[i], finger), cp->fingers[i]);
    //     }
    // }
    // printf("Cache end.\n");

    P(&cp->readable);
    P(&cp->mutex);
    if((++cp->read_count) == 1) P(&cp->writable);
    V(&cp->mutex);
    V(&cp->readable);   

    int index = -1;
    for(int i =0; i < cp->num_obj; i++){
        if((cp->valids[i]) && (strcmp(cp->fingers[i], finger) == 0)){
            index = i;
            *lengthp = cp->obj_sizes[i];
            memcpy(dest, cp->objects[i], *lengthp);
            break;
        }
    }

    P(&cp->mutex);
    if(index >= 0) cp->timestamps[index] = (cp->global_time)++;
    // global_time circled, reset all the timestmap
    if(cp->global_time == 0){
        cp->global_time = 1;
        for(int i = 0; i < cp->num_obj; i++){
            cp->timestamps[i] = cp->global_time ++;
        }
    }
    if((--(cp->read_count)) == 0) V(&cp->writable);
    V(&cp->mutex);

    return index;
}

void store_obj(cache_t *cp, char *finger, char* content, size_t length){

    // printf("Showing cache:\n");
    // for(int i = 0; i<cp->num_obj; i++){
    //     if (cp->valids[i]){
    //         printf("%d: %ld %s\n", i, cp->obj_sizes[i], cp->fingers[i]);
    //     }
    // }
    // printf("Cache end.\n");

    //printf("0\n");
    P(&cp->readable);
    P(&cp->writable);
    //printf("1\n");
    // find least recently used block.
    int index = 0, time_stamp = cp->global_time;
    for(int i =0; i<cp->num_obj; i++){
        if(cp->valids[i]){
            if(cp->timestamps[i] <= time_stamp){
                time_stamp = cp->timestamps[i];
                index = i;
            }
        }
        else{
            index = i;
            break;
        }
    }
    // store object at index
    strcpy(cp->fingers[index], finger);
    memcpy(cp->objects[index], content, length);
    cp->valids[index] = 1;
    cp->obj_sizes[index] = length;

    //P(&cp->mutex);
    cp->timestamps[index] = (cp->global_time)++;
    // global_time circled, reset all the timestmap
    if(cp->global_time == 0){
        cp->global_time = 1;
        for(int i = 0; i < cp->num_obj; i++){
            cp->timestamps[i] = cp->global_time ++;
        }
    }
    //V(&cp->mutex);
    //printf("3\n");
    V(&cp->writable);
    V(&cp->readable);
    //printf("4\n");
}

void cache_destory(cache_t *cp){
    for(int i = 0; i < cp->num_obj; i++){
        Free(cp->fingers[i]);
        Free(cp->objects[i]);
    }
    Free(cp->fingers);
    Free(cp->objects);
    Free(cp->timestamps);
    Free(cp->valids);
    Free(cp->obj_sizes);
}
