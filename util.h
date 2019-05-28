#pragma once

#include <stdint.h>

// Returns a pointer to the current data break.
void *get_break();

// Moves the data break, positive argument to increase memory and negative to
// decrease.
// Return value is the new data break, or NULL if a failure occurred.
void *move_break(long int);

// Create a 64 bit bitmask. Ones stored in the lower bits.
uint64_t bitmask(int);

// Print a bit array, one byte in binary to a line.
void print_bit_array(uint8_t *, int);
