#include "spmalloc.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

#define BLOCK_SIZE (0x1000)
#define MIN_ALLOCABLE (4)
#define TBL_SIZE ((BLOCK_SIZE-sizeof(struct blk_info))/((MIN_ALLOCABLE*8)+1))

struct blk_info {
	// Next block of data
	struct blk_info *next;
	// Bitmap table. Each bit corresponds to a four-byte chunk in the block that
	// comes immediately after this table.
	uint8_t tbl[];
};

struct tag {
	size_t size; // includes the tag size.
};

// dhead stores the head of our linked list of data.
static struct blk_info *dhead;

void
blk_clear(struct blk_info *b) {
	b->next = NULL;
	memset(b->tbl, 0xFF, TBL_SIZE);
}

void *
blk_find(struct blk_info *b, size_t size) {
	uint64_t mask, shift_mask;
	struct tag *chunk;

	// Size needs to include the tag, and be a factor of 4 bytes.
	size += sizeof(struct tag);
	size = (size >> 2) + ((size & 0x3) != 0);

	// Construct a mask for the number of chunks we need
	mask = bitmask(size);

	// For each bitmap table entry
	for (unsigned i = 0; i < TBL_SIZE; i++) {
		shift_mask = mask;
		for (unsigned off = 0; off < 8; off++) {
			if ((*((uint64_t *) b->tbl+i) & shift_mask) == shift_mask) {
				// Found a suitable chunk at the ith block of 4*8 bytes plus
				// `off` * 4 bytes.
				chunk = (struct tag *) ((uint8_t *)b + sizeof(struct blk_info) + TBL_SIZE + i*8*4 + off*4);
				chunk->size = size;
				*((uint64_t *) (b->tbl+i)) &= ~shift_mask;
				return (uint8_t*)chunk + sizeof(struct tag);
			}

			// Search one bit over
			shift_mask <<= 1;
		}
	}

	return NULL;
}

void
blk_release(struct blk_info *b, void *ptr) {
	struct tag *chunk;
	size_t size;
	unsigned offset, bitoff, byteoff;
	uint64_t mask;

	chunk = (struct tag *)((uint8_t *) ptr - sizeof(struct tag));
	size = chunk->size;

	// Calculate indices to the bit array.
	// First we need the actual number of bytes ptr is offset into the block.
	// Then we convert the offset into a 4-byte offset.
	// The byte offset in the bitmap is then that number divided by 8 bits,
	// and the bit offset is the modulo over 8.
	offset = (uint8_t *)chunk - ((uint8_t *)b+sizeof(struct blk_info)+TBL_SIZE);
	offset >>= 2;
	byteoff = offset >> 3;
	bitoff  = offset & 0x7;

	// Set the mask up. The mask is shifted to account for the bit offset inside
	// the bit array.
	mask = bitmask(size);
	mask <<= bitoff;

	// And perform the wipe!
	*((uint64_t*) (b->tbl + byteoff)) |= mask;
}

void *
malloc(size_t size) {
	struct blk_info *walk;
	void *match;

	if (size == 0) {
		return NULL; // per the standard
	}
	else if (((size + sizeof(struct tag))>>2) > 64-8) {
		// Our bitmasking solution cannot accommodate chunks this large.
		// Bitmasks need to be able to shift over 8 bits to check against every
		// position of a given byte.
		// TODO -- perhaps use mmap here
		errno = ENOTSUP;
		return NULL;
	}

	if (dhead == NULL) {
		dhead = move_break(BLOCK_SIZE);
		if (dhead == NULL) {
			return NULL;
		}
		dhead = (struct blk_info *)((uint8_t *) dhead - BLOCK_SIZE);
		blk_clear(dhead);
	}

	for (walk = dhead; walk != NULL; walk = walk->next) {
		match = blk_find(walk, size);
		if (match != NULL) {
			// Found a suitable hole in a block we already had allocated.
			// Give this data segment to the caller.
			return match;
		}

		// If we are at the end of the block list with no match, lay down
		// another block for the next iteration.
		if (walk->next == NULL) {
			walk->next = move_break(BLOCK_SIZE);
			if (walk->next == NULL) {
				return NULL;
			}
			walk->next = (struct blk_info *)((uint8_t *) walk->next - BLOCK_SIZE);
			blk_clear(walk->next);
		}
	}

	// Never executed.
	return NULL;
}

void *
realloc(void *ptr, size_t size) {
	// TODO
	(void) ptr;
	(void) size;
	return NULL;
}

void
free(void *ptr) {
	struct blk_info *walk;

	// Walk over the blocks. If the ptr we are freeing is greater than the
	// address to a given block, the ptr must be in that block.
	// We know this because the blocks are ordered in increasing memory address
	// on systems where the heap grows up.
	for (walk = dhead; walk != NULL; walk = walk->next) {
		if (ptr > (void *) walk) {
			blk_release(walk, ptr);
			return;
		}
	}

	// Caller tried to release memory we do not know of.
	return;
}
