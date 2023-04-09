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

static char *heap_listp; // 처음에 쓸 가용블록을 생성

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
