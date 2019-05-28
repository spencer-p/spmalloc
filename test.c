#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "spmalloc.h"


int main() {
	uint8_t *x = malloc(1);
	uint8_t *y = malloc(1);
	*y = 0x1;
	*x = 0xff;
	printf("x, y = %d, %d\n", *x, *y);
	free(x);
	free(y);

	x = malloc(127);
	if (!x) perror("malloc");
	free(x);

	x = malloc(0x800);
	memset(x, 7, 256);
	free(x);
}
