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
#define EPREV_BLKP(bp) (GET((char *)(bp) + WSIZE))
#define ENEXT_BLKP(bp) (GET(bp))
/* Set the next and prev block of empty block in empty block chain */
#define ESET_PREV_BLK(bp, nbp) (*(unsigned int *)(((char *)(bp) + WSIZE)) = (unsigned int)(nbp)) 
#define ESET_NEXT_BLK(bp, pbp) (*(unsigned int *)(bp) = (unsigned int)(pbp))

#define DEBUG 1

#ifdef DEBUG
#define CHECK() mm_checkheap();
#else
#define CHECK()
#endif

#ifdef DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static void *heap_listp, *true_header; 

static inline unsigned int *chain_header(size_t size);
static inline void remove_from_chain(void * bp);
static inline void add_to_chain(void * bp);
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
int mm_checkheap(void);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    PRINTF("\n\n-------------------------\ninit\n");

    if ((true_header = mem_sbrk(ADDRESSLEN*WSIZE + 4*WSIZE)) == (void *) -1)
        return -1;

    char *tail = (char *) (true_header) + ADDRESSLEN*WSIZE;
    for(char *i = (char*)(true_header) + 2*WSIZE; i < tail; i+=WSIZE){
        ESET_NEXT_BLK(i, true_header);
    }

    heap_listp = true_header + ADDRESSLEN*WSIZE;
    PUT(heap_listp, 0);
    PUT(heap_listp+WSIZE, PACK(DSIZE,1));
    PUT(heap_listp+2*WSIZE, PACK(DSIZE,1));
    PUT(heap_listp+3*WSIZE, PACK(0,1));

    heap_listp += (2*WSIZE);

    CHECK();

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    CHECK();

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    PRINTF("\nmalloc: %d\n", size);
    CHECK();
    
    size_t asize;
    char *bp;
    size_t extendsize;

    if (size == 0)
        return NULL;
    
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);

    if((bp = find_fit(asize))==NULL){
        extendsize = MAX(asize, CHUNKSIZE);
        if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    }

    place(bp, asize);
    CHECK();
    return bp;
}

/*
 * mm_free
 */
void mm_free(void *bp)
{
    PRINTF("free: %p\n", bp);
    CHECK();

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
    CHECK();
}

/*
 * mm_realloc - return the new address if success, otherwise return NULL.
 */
void *mm_realloc(void *bp, size_t size)
{
    PRINTF("relloc: %p, %d\n", bp, size);
    CHECK();

    if (bp == NULL)
        return mm_malloc(size);

    if (size == 0){
        mm_free(bp);
        CHECK();
        return NULL;
    }

    void * final_bp;
    // the adjusted size needed for new block.
    size_t asize;
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);

    size_t origin_size = GET_SIZE(HDRP(bp));
    int delta = asize - origin_size;

    // need to expand block
    if (delta >0){
        char *nbp;
        unsigned int next_size;

        nbp = (char *)NEXT_BLKP(pb);
        next_size = (unsigned int) GET_SIZE(HDRP(nbp));

        // if the next block is empty block and the total size is enough
        if ((!GET_ALLOC(HDRP(nbp))) && next_size >= delta){
            remove_from_chain(nbp);
            if (next_size - delta >= 2*DSIZE){
                // truncate next block
                PUT(HDRP(bp), PACK(asize, 1));
                PUT(FTRP(bp), PACK(asize, 1));
                PUT(HDRP(NEXT_BLKP(bp)), PACK(next_size-delta, 0));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(next_size-delta, 0));
                coalesce(NEXT_BLKP(bp));
            }
            else{
                // add next block directly
                asize = origin_size + next_size;
                PUT(HDRP(bp), PACK(asize, 1));
                PUT(FTRP(bp), PACK(asize, 1));
            }
            //return bp;
            final_bp = bp;
        }
        else{
            nbp = find_fit(asize);
            if (nbp == NULL){
                size_t extendsize = MAX(asize, CHUNKSIZE);
                if((nbp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
            }
            place(nbp, asize);
            memcpy(nbp, bp, origin_size-2);
            free(bp);
            //return nbp;
            final_bp = nbp;
        }
    }
    // 0 -- -(2*DSIZE-1), do nothing.
    else if(delta >= -(2*DSIZE - 1)){
        final_bp = bp;
    }
    // -2*DSIZE -- .. , just truncate the original block.
    else{
        PUT(FTRP(bp), PACK(delta, 0));
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(delta, 0));
        coalesce(NEXT_BLKP(bp));
        final_bp =  bp;
    }

    CHECK();
    return final_bp;
}

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


/* coalesce empty block and add to chain, the empty block is not in the empty block chain. */
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
    PRINTF("extend heap\n");
    char* bp;
    size_t size;

    size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    bp = coalesce(bp);
    CHECK();
    return bp;
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
    size_t rsize = GET_SIZE(HDRP(bp)) - size;

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

int mm_checkheap(void){
    printf("check: ");

    char * bp = heap_listp;
    int num_eblock = 0, num_ablock = 0;

    // prinf the heap
    int i = 0;
    while(GET_SIZE(HDRP(bp)) && i<50000){
        // check the structure of block
        assert (*(unsigned int *)HDRP(bp) == * (unsigned int *)FTRP(bp));
        if (GET_ALLOC(HDRP(bp))) num_ablock++;
        else num_eblock ++;

        printf("%05d/%d\t", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
        bp = NEXT_BLKP(bp);
        i++;
    }
    printf("%05d/%d\t", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
    num_ablock ++;
    printf("\t");
    printf("alloced block: %d, empty block: %d\n", num_ablock, num_eblock);

    // check the empty block chain
    char *header;
    for(int group = 2; group < ADDRESSLEN; group ++){
        header = (char *)(true_header) + group*WSIZE;
        bp = header;
        while((bp = (char *)ENEXT_BLKP(bp)) != (char *)(true_header)){
            assert ((char *)ENEXT_BLKP(EPREV_BLKP(bp)) == bp);
            assert ((char *)EPREV_BLKP(ENEXT_BLKP(bp)) == bp
             || (char *)ENEXT_BLKP(bp) == (char *)(true_header));
            assert (((char *)chain_header(GET_SIZE(HDRP(bp)))) == header);

            num_eblock --;
        }
    }
    assert (num_eblock == 0);

    return 0;
}












