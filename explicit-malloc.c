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

#define WSIZE 4             // word size
#define DSIZE 8             // double word size
#define CHUNKSIZE (1 << 12) // heap을 한번 extend할 때 늘리는 용량 (약 4kb)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc)) // header와 footer에 블록 정보를 넣기 위함

#define GET(p) (*(unsigned int *)(p))                   // 다른 block의 주소를 얻어옴
#define PUT(p, val) (*(unsigned int *)(p) = (int)(val)) // block의 주소를 넣음

#define GET_SIZE(p) (GET(p) & ~0x7) // get으로 다른 block의 주소를 얻어와 해당 블록의 size만 얻어옴 (~는 역수를 의미하므로 ~0x7은 11111000이 됨. 비트 연산을 통해 맨 뒤 세자리를 제외한 정보를 가져올 수 있게 됨.)
#define GET_ALLOC(p) (GET(p) & 0x1) // get으로 다른 block의 주소를 얻어와 해당 블록의 alloc(가용여부)를 얻어옴

#define HDRP(bp) ((char *)(bp)-WSIZE)                        // bp는 header다음에 위치하므로 WSIZE를 빼줘서 header를 가르키게 함
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // bp가 가리키는 block의 header로 이동해 해당 block의 사이즈만큼 이동하고 DSIZE를 빼주어 footer를 가르키게 함

#define NEXT_HDRP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) // bp가 가리키는 block의 header로 이동해 해당 block의 사이즈만 큼 이동 -> 다음 block의 header를 가리키게 됨
#define PREV_FTRP(bp) (char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE))     // bp는 block의 header 다음을 카리키고 있으므로 DSIZE를 빼서 이전 block의 footer로 가서 size를 가져와 빼줌. 이후 이전 block의 헤더 다음을 가리키게 함

// explicit 방식에서 추간된 매크로
#define PREV_FREEP(bp) (*(void **)(bp))         // 현재 블록 전 가용 블록
#define NEXT_FREEP(bp) (*(void **)(bp + WSIZE)) // 현재 블록 다음 가용 블록

static char *heap_listp;
static char *free_listp; // 가용블록들만 저장할 free_listp 생성

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void put_freelist(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void remove_in_freelist(void *bp);

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *)-1)
    {
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE * 2, 1)); // implicit에선 prolog header, prolog footer만 있엇기에 dsize였지만, explicit에선 전위자(p), 후위자(s)가 있음
    PUT(heap_listp + (2 * WSIZE), NULL);               // predecessor
    PUT(heap_listp + (3 * WSIZE), NULL);               // successor
    PUT(heap_listp + (4 * WSIZE), PACK(DSIZE * 2, 1));
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    free_listp = heap_listp;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size <= 0)
        return NULL;

    if (size <= DSIZE)
    {
        asize = 2 * DSIZE;
    }
    else
    {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    else
    {
        extendsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        {
            return NULL;
        }
        place(bp, asize);
        return bp;
    }
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

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
        put_freelist(bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        remove_in_freelist(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_in_freelist(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else
    {
        remove_in_freelist(PREV_BLKP(bp));
        remove_in_freelist(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    put_freelist(bp);
    return bp;
}

static void put_freelist(void *bp)
{
    NEXT_FREEP(bp) = free_listp; // 현재 block NEXT freeblock을 카리키는 포인터는 free_listp
    PREV_FREEP(bp) = NULL;       // 현재 block이 스택에서 가장 마지막 block일테니 현재 블록의 PREV는 NULL
    PREV_FREEP(free_listp) = bp; // free_listp의 PREV는 현재 block
    /*
         prev                   next
    ------------------------------------
             PREV_FREEP(free_listp)    |
 NULL <---  ------   <---    ------    |
           |  24  |         |  8   |   |
            ------   --->    ------    |
                NEXT_FREEP(bp)         |
    ------------------------------------
    */
    free_listp = bp; // free_listp가 stack에서 가장 최근에 들어온 block을 가리기게 함
}

static void *find_fit(size_t asize)
{
    void *bp;
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)) // free_listp의 block들을 순회
    {
        if (GET_SIZE(HDRP(bp)) >= asize) // size가 적합할 시에 해당 bp를 반환
        {
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    remove_in_freelist(bp); // bp가 가리키는 블록은 더이상 가용블록이 아니므로 freelist에서 삭제
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        put_front_of_freelist(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

static void remove_in_freelist(void *bp)
{
    if (bp == free_listp) // bp가 stack 첫(가장 최근에 들어온) 블록일 경우
    {
        PREV_FREEP(NEXT_FREEP(bp)) = NULL;
        free_listp = NEXT_FREEP(bp);
    }
    else // 가운데 블록일 경우
    {
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp); // linked list 중간 삭제 처럼 bp의 전후를 이어주는 과정
        PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
    }
}