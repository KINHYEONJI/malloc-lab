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
    "jungle6_week06_6",
    /* First member’s full name */
    "KimHyeonji",
    /* First member’s email address */
    "hyeonji0718 @gmail.com",
    /* Second member’s full name (leave blank if none) */
    "",
    /* Second member’s email address (leave blank if none) */
    ""};

#define WSIZE sizeof(void *) // pointer의 크기 (32bit -> WSIZE = 4 / 64bit -> WSIZE = 8)
#define DSIZE (2 * WSIZE)    // double word size
#define MINIMUM 8
#define CHUNKSIZE (1 << 12)
#define LISTLIMIT 20

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(ptr) ((char *)(ptr)-WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE(((char *)(ptr)-WSIZE)))
#define PREV_BLKP(ptr) ((char *)(ptr)-GET_SIZE(((char *)(ptr)-DSIZE)))

// segreagated-fit 방식에서 추가된 메크로 (prev, next 보다는 predecessor, successor이 직관적)
#define PREC_FREEP(ptr) (*(void **)(ptr))
#define SUCC_FREEP(ptr) (*(void **)(ptr + WSIZE))

static char *heap_listp;
static void *seg_list[LISTLIMIT];

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void put_block_in_seglist(void *bp, size_t size);
static void remove_block_in_seglist(void *bp);

int mm_init(void)
{
    for (int i = 0; i < LISTLIMIT; i++)
    {
        seg_list[i] = NULL;
    }
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(MINIMUM, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(MINIMUM, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp = heap_listp + 2 * WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : (words)*WSIZE;

    if (((bp = mem_sbrk(size)) == (void *)-1))
    {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        put_block_in_seglist(bp, size);
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block_in_seglist(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        remove_block_in_seglist(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        remove_block_in_seglist(PREV_BLKP(bp));
        remove_block_in_seglist(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        bp = PREV_BLKP(bp);
    }

    put_block_in_seglist(bp, size);
    return bp;
}

static void put_block_in_seglist(void *bp, size_t size)
{
    int idx = 0;
    void *search_ptr;
    void *insert_ptr = NULL; // 두 번째 while문에서 사용. search_ptr의 값을 저장해놓는 용도

    while ((idx < LISTLIMIT - 1) && (size > 1)) // size가 들어갈 수 있는 seglist index를 찾는 과정
    {
        size >>= 1; // size의 비트를 1씩 오른쪽으로 shift 시키고
        idx++;      // idx를 하나씩 증가 시켜 해당 블록이 들어갈 seg_list index찾음
    }

    search_ptr = seg_list[idx];

    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) // seg_list[idx]에서 몇 번째 원소가 될지 찾는 과정
    {
        insert_ptr = search_ptr;
        search_ptr = SUCC_FREEP(search_ptr);
    }

    if (search_ptr != NULL)
    {
        if (insert_ptr != NULL) // case1 : seg_list[idx]의 중간에 넣을 때
        {
            SUCC_FREEP(bp) = search_ptr;
            PREC_FREEP(bp) = insert_ptr;
            PREC_FREEP(search_ptr) = bp;
            SUCC_FREEP(insert_ptr) = bp;
        }
        else // case2 : seg_list[idx]의 제일 앞에 넣을 때
        {
            SUCC_FREEP(bp) = search_ptr;
            PREC_FREEP(bp) = NULL;
            PREC_FREEP(search_ptr) = bp;
            seg_list[idx] = bp;
        }
    }
    else
    {
        if (insert_ptr != NULL) // case3 : seg_list[idx]의 제일 마지막에 넣을 때
        {
            SUCC_FREEP(bp) = NULL;
            PREC_FREEP(bp) = insert_ptr;
            SUCC_FREEP(insert_ptr) = bp;
        }
        else // case4 : seg_list[idx]가 NULL일 때 (아무것도 없고 해당 블록이 첫 원소가 될 때)
        {
            SUCC_FREEP(bp) = NULL;
            PREC_FREEP(bp) = NULL;
            seg_list[idx] = bp;
        }
    }
    return;
}

static void remove_block_in_seglist(void *bp)
{
    int idx = 0;
    size_t size = GET_SIZE(HDRP(bp));

    while ((idx < LISTLIMIT - 1) && (size > 1)) // size가 들어갈 수 있는 seglist index를 찾는 과정
    {
        size >>= 1; // size의 비트를 1씩 오른쪽으로 shift 시키고
        idx++;      // idx를 하나씩 증가 시켜 해당 블록이 들어갈 seg_list index찾음
    }

    if (SUCC_FREEP(bp) != NULL)
    {
        if (PREC_FREEP(bp) != NULL) // case1 : 지우려는 block이 가운데에 있을 경우
        {
            PREC_FREEP(SUCC_FREEP(bp)) = PREC_FREEP(bp);
            SUCC_FREEP(PREC_FREEP(bp)) = SUCC_FREEP(bp);
        }
        else // case2 : 지우려는 block이 제일 앞일 경우
        {
            PREC_FREEP(SUCC_FREEP(bp)) = NULL;
            seg_list[idx] = SUCC_FREEP(bp);
        }
    }
    else
    {
        if (PREC_FREEP(bp) != NULL) // case3 : 지우려는 block이 제일 뒤일 경우
        {
            SUCC_FREEP(PREC_FREEP(bp)) = NULL;
        }
        else // case4 : 지우려는 block이 해당 seg_list[idx]에 홀로 있을 경우
        {
            seg_list[idx] = NULL;
        }
    }
    return;
}
