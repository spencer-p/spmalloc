# Spencer P's Malloc

This is toy implementation of the C standard library `malloc` family of
functions. It aims to be simple and freestanding.

The internals are implemented with a bit array that represents free half words
(on 64-bit Linux). Allocation & deallocation is performed via scanning the
bitmaps within each larger chunk of the data segment.
