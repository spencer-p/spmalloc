#include "util.h"

#include <errno.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>

static void *cur_break;

void *
get_break() {
	if (cur_break == NULL) {
		// If we don't know the break, try to change it to the zero address.
		// This will fail and the syscall will return the actual data break.
		cur_break = (void *) syscall(SYS_brk, (void *)0);
	}
	return cur_break;
}

void *
move_break(long int offs) {
	void *old_break;

	old_break = get_break();
	cur_break = (void *) syscall(SYS_brk, old_break + offs);

	if (cur_break == old_break) {
		// If Linux did not move the break, we do not have new memory.
		log("brk syscall failed");
		errno = ENOMEM;
		return NULL;
	}

	return cur_break;
}

uint64_t 
bitmask(int len) {
	uint64_t mask = 0;
	for (; len > 0; len--) {
		mask <<= 1;
		mask |= (uint64_t) 1;
	}
	return mask;
}

void
print_bit_array(uint8_t *b, int len) {
	char buf[9];
	buf[8] = '\n';
	for (int i = 0; i < len; i++) {
		uint8_t bi = b[i];
		for (int j = 0; j < 8; j++) {
			buf[j] = ((bi & (uint8_t)0x80) != 0) + '0';
			bi <<= 1;
		}
		write(1, buf, 9);
	}
	write(1, buf+8, 1);
}
