// SPDX-License-Identifier: BSD-3-Clause

#include <sys/mman.h>
#include <unistd.h>

#include "osmem.h"
#include "block_meta.h"

/* All payloads and headers are aligned to 8 bytes (64-bit systems). */
#define ALIGNMENT	8
#define ALIGN(size)	(((size) + (ALIGNMENT - 1)) & ~((size_t)ALIGNMENT - 1))

/* Size of the block header, rounded up so the payload stays aligned. */
#define META_SIZE	ALIGN(sizeof(struct block_meta))

/* Blocks (header + payload) at least this big are served with mmap(). */
#define MMAP_THRESHOLD	(128 * 1024)

/* Size of the chunk reserved with brk() on the first heap allocation. */
#define HEAP_PREALLOC	(128 * 1024)

/* First block of the heap; blocks form a doubly linked list in address order. */
static struct block_meta *heap_start;

/* Header that precedes a payload returned to the user. */
static struct block_meta *get_block(void *ptr)
{
	return (struct block_meta *)((char *)ptr - META_SIZE);
}

/* Payload address handed to the user for a given block. */
static void *get_payload(struct block_meta *block)
{
	return (char *)block + META_SIZE;
}

/*
 * Allocate a standalone block with mmap(). Mapped blocks are not part
 * of the heap list: each one is released independently with munmap().
 */
static struct block_meta *alloc_mapped(size_t size)
{
	struct block_meta *block;

	block = mmap(NULL, META_SIZE + size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	DIE(block == MAP_FAILED, "mmap");

	block->size = size;
	block->status = STATUS_MAPPED;
	block->prev = NULL;
	block->next = NULL;

	return block;
}

static void free_mapped(struct block_meta *block)
{
	int ret = munmap(block, META_SIZE + block->size);

	DIE(ret == -1, "munmap");
}

/* Move the program break up by @increment bytes; returns the old break. */
static void *heap_grow(size_t increment)
{
	void *old_brk = sbrk(increment);

	DIE(old_brk == (void *)-1, "sbrk");

	return old_brk;
}

/*
 * Reserve HEAP_PREALLOC bytes on the first heap use, so that later small
 * allocations do not need a brk() call each. The whole chunk becomes a
 * single free block.
 */
static void heap_preallocate(void)
{
	heap_start = heap_grow(HEAP_PREALLOC);
	heap_start->size = HEAP_PREALLOC - META_SIZE;
	heap_start->status = STATUS_FREE;
	heap_start->prev = NULL;
	heap_start->next = NULL;
}

static struct block_meta *heap_last(void)
{
	struct block_meta *block = heap_start;

	while (block && block->next)
		block = block->next;

	return block;
}

/* First free heap block that can hold @size bytes, or NULL. */
static struct block_meta *find_free_block(size_t size)
{
	struct block_meta *block;

	for (block = heap_start; block; block = block->next)
		if (block->status == STATUS_FREE && block->size >= size)
			return block;

	return NULL;
}

/* Add a new block of @size bytes at the end of the heap. */
static struct block_meta *heap_append(size_t size)
{
	struct block_meta *last = heap_last();
	struct block_meta *block = heap_grow(META_SIZE + size);

	block->size = size;
	block->status = STATUS_ALLOC;
	block->prev = last;
	block->next = NULL;
	last->next = block;

	return block;
}

static struct block_meta *alloc_heap(size_t size)
{
	struct block_meta *block;

	if (!heap_start)
		heap_preallocate();

	block = find_free_block(size);
	if (block) {
		block->status = STATUS_ALLOC;
		return block;
	}

	return heap_append(size);
}

void *os_malloc(size_t size)
{
	if (size == 0)
		return NULL;

	size = ALIGN(size);

	if (META_SIZE + size >= MMAP_THRESHOLD)
		return get_payload(alloc_mapped(size));

	return get_payload(alloc_heap(size));
}

void os_free(void *ptr)
{
	struct block_meta *block;

	if (!ptr)
		return;

	block = get_block(ptr);

	if (block->status == STATUS_MAPPED)
		free_mapped(block);
	else if (block->status == STATUS_ALLOC)
		/* Heap memory is kept for reuse; the break is never lowered. */
		block->status = STATUS_FREE;
}

void *os_calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement os_calloc */
	(void)nmemb;
	(void)size;
	return NULL;
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
	(void)ptr;
	(void)size;
	return NULL;
}
