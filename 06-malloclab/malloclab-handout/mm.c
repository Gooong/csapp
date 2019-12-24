/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Team Alone",
    /* First member's full name */
    "Xuri Gong",
    /* First member's email address */
    "gongxuri@pku.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define ALIGNMENT 8
#define ADDRESSLEN 32

#define MAX(x,y) ((x) > (y)? (x) : (y))
/* Pack a size and allocated bit into a word*/
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(pb) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* Get the next and prev block of empty block in empty block chain */
#define ENEXT_BLKP(bp) (GET((char *)(bp) + WSIZE))
#define EPREV_BLKP(bp) (GET(bp))
/* Set the next and prev block of empty block in empty block chain */
#define ESET_NEXT_BLK(bp, nbp) (*(unsigned int *)(((char *)(bp) + WSIZE)) = (unsigned int)(nbp)) 
#define ESET_PREV_BLK(bp, pbp) (*(unsigned int *)(bp) = (unsigned int)(pbp))

static void *heap_listp, *true_header; 

/* Get the header of empty block chain */
static inline unsigned int *chain_header(size_t size){
    return ((unsigned int *)(true_header) + (ADDRESSLEN - __builtin_clz(size)));
}

static inline void remove_from_chain(void * bp){
    ESET_NEXT_BLK(EPREV_BLKP(bp), ENEXT_BLKP(bp));
    ESET_PREV_BLK(ENEXT_BLKP(bp), EPREV_BLKP(bp));
}

static inline void add_to_chain(void * bp){
    size_t size = GET_SIZE(HDRP(bp));
    unsigned int *headerp = chain_header(size);
    
    ESET_NEXT_BLK(bp, ENEXT_BLKP(headerp));
    ESET_PREV_BLK(bp, headerp);
    ESET_PREV_BLK(ENEXT_BLKP(headerp), bp);
    ESET_NEXT_BLK(headerp, bp);
}

/* bp is not in the empty block chain */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        ;
    }
    
    else if(prev_alloc && !next_alloc){
        remove_from_chain(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if(!prev_alloc && next_alloc){
        remove_from_chain(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else{
        remove_from_chain(PREV_BLKP(bp));
        remove_from_chain(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    add_to_chain(bp);
    return bp;
}

static void *extend_heap(size_t words)
{
    char* bp;
    size_t size;

    size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}


static void *find_fit(size_t size){
    char *header = (char *)(chain_header(size));
    char *tail = (char *) (true_header) + ADDRESSLEN*WSIZE;
    char *bp;
    for(; header < tail; header+=WSIZE){
        bp = header;
        while((bp = (char *)ENEXT_BLKP(bp)) != (char *)(true_header)){
            if (GET_SIZE(FTRP(bp))>=size) return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t size){
    size_t rsize = GET_SIZE(FTRP(bp)) - size;

    if (rsize < 2*DSIZE){
        size += rsize;
        rsize = 0;
    }

    remove_from_chain(bp);
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));

    if(rsize!=0){
        char *rbp = NEXT_BLKP(bp);
        PUT(HDRP(rbp), PACK(rsize, 0));
        PUT(FTRP(rbp), PACK(rsize, 0));
        add_to_chain(rbp);
    }
}

int mm_check(void){
    void * bp = heap_listp;
    while(GET_SIZE(FTRP(bp))!=0){
        assert (*(unsigned int *)FTRP(bp) == * (unsigned int *)FTRP(bp));
        printf("%04d/%d\t", GET_SIZE(FTRP(bp)), GET_ALLOC(FTRP(bp)));
        bp = NEXT_BLKP(bp);
    }
    return 0;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    printf("init");
    mm_check();

    if ((true_header = mem_sbrk(ADDRESSLEN*WSIZE + 4*WSIZE)) == (void *) -1)
        return -1;

    char *tail = (char *) (true_header) + ADDRESSLEN*WSIZE;
    for(char *i = (char*)(true_header) + 2*WSIZE; i < tail; i+=WSIZE){
        ESET_NEXT_BLK(i, true_header);
    }

    heap_listp += ADDRESSLEN*WSIZE;
    PUT(heap_listp, 0);
    PUT(heap_listp+WSIZE, PACK(DSIZE,1));
    PUT(heap_listp+2*WSIZE, PACK(DSIZE,1));
    PUT(heap_listp+3*WSIZE, PACK(0,1));

    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    printf("malloc: %d", size);
    mm_check();
    
    size_t asize;
    char *bp;
    size_t extendsize;

    if (size == 0)
        return NULL;
    
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);
    
    if((bp = find_fit(asize))!=NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    bp = coalesce(bp);
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    printf("free: %d", bp);
    mm_check();

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    printf("relloc: %d, %d", ptr, size);
    mm_check();

    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0){
        mm_free(ptr);
    }
    return NULL;
}














