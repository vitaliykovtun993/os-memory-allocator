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

void *os_malloc(size_t size)
{
	if (size == 0)
		return NULL;

	size = ALIGN(size);

	if (META_SIZE + size >= MMAP_THRESHOLD)
		return get_payload(alloc_mapped(size));

	/* TODO: small allocations on the heap with brk() */
	return NULL;
}

void os_free(void *ptr)
{
	struct block_meta *block;

	if (!ptr)
		return;

	block = get_block(ptr);

	if (block->status == STATUS_MAPPED)
		free_mapped(block);

	/* TODO: release heap blocks */
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
